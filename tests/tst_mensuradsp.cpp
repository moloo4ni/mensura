#include "clicksynth.h"
#include "mensuradsp.h"
#include "sharedstate.h"

#include <core/engine/audioformat.h>
#include <core/engine/dsp/processingbuffer.h>
#include <core/engine/dsp/processingbufferlist.h>

#include <QTest>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace Fooyin;
using namespace Fooyin::Mensura;
using namespace Qt::StringLiterals;

namespace {
constexpr int Rate = 48000;

MensuraParams enabledParams()
{
    MensuraParams params;
    params.enabled     = true;
    params.bpm         = 120.0;
    params.beatsPerBar = 4;
    params.sound       = ClickSound::Click;
    params.gain        = 0.5;
    params.phaseNs     = 0;
    return params;
}

std::vector<double> run(MensuraDsp& dsp, const std::vector<double>& input, int channels, uint64_t startNs,
                        int rate = Rate, SampleFormat format = SampleFormat::F64)
{
    ProcessingBufferList list;
    list.setToSingle(ProcessingBuffer{input, AudioFormat{format, rate, channels}, startNs});
    dsp.process(list);
    if(list.count() == 0) {
        return {};
    }
    const auto output = list.item(0)->constData();
    return {output.begin(), output.end()};
}

uint64_t exactStartNs(size_t frame, int rate)
{
    return static_cast<uint64_t>(std::llround(static_cast<double>(frame) * 1e9 / rate));
}

// fooyin quantises per-track buffer start times to whole milliseconds
uint64_t msQuantisedStartNs(size_t frame, int rate)
{
    return static_cast<uint64_t>(frame * 1000 / static_cast<size_t>(rate)) * 1'000'000;
}

std::vector<double> runChunked(MensuraDsp& dsp, const std::vector<double>& input, int channels, int rate,
                               const std::vector<int>& chunkFrames, uint64_t (*startNs)(size_t, int),
                               uint64_t trackOffsetNs = 0)
{
    const size_t totalFrames = input.size() / static_cast<size_t>(channels);
    std::vector<double> output;
    size_t offset{0};
    for(size_t index = 0; offset < totalFrames; ++index) {
        const size_t frames = std::min<size_t>(static_cast<size_t>(chunkFrames[index % chunkFrames.size()]),
                                               totalFrames - offset);
        const auto begin    = input.begin() + static_cast<std::ptrdiff_t>(offset * channels);
        const std::vector<double> chunk(begin, begin + static_cast<std::ptrdiff_t>(frames * channels));
        const auto processed = run(dsp, chunk, channels, trackOffsetNs + startNs(offset, rate), rate);
        output.insert(output.end(), processed.begin(), processed.end());
        offset += frames;
    }
    return output;
}

// Checks out[(startFrame + k) * channels + ch] == click[k] * gain for every k and channel
bool hasClickAt(const std::vector<double>& out, int channels, size_t startFrame, std::span<const double> click,
                double gain, size_t skip = 0)
{
    for(size_t k = skip; k < click.size(); ++k) {
        for(int ch = 0; ch < channels; ++ch) {
            const size_t index = (startFrame + k - skip) * static_cast<size_t>(channels) + static_cast<size_t>(ch);
            if(index >= out.size() || out[index] != click[k] * gain) {
                return false;
            }
        }
    }
    return true;
}

bool isSilent(const std::vector<double>& out, int channels, size_t fromFrame, size_t toFrame)
{
    for(size_t i = fromFrame * static_cast<size_t>(channels); i < toFrame * static_cast<size_t>(channels); ++i) {
        if(out[i] != 0.0) {
            return false;
        }
    }
    return true;
}
} // namespace

class TestMensuraDsp : public QObject
{
    Q_OBJECT

private slots:
    void identity();
    void disabledLeavesAudioUntouched();
    void clickStartsOnExactFrame();
    void tailContinuesAcrossMsQuantisedBuffers();
    void tinyAndEmptyBuffersMatchOneBigBuffer();
    void seekDropsTail();
    void resetDropsTail();
    void accentOnlyOnDownbeat();
    void singleBeatBarHasNoAccent();
    void heartbeatUpdatedOnProcess();
    void mixesIntoEveryChannel();
    void softClipBoundsOutput();
    void sampleRateChangeWithoutPrepare();
    void twoInstancesKeepSeparateTimelines();
    void disablingMidClickFinishesIt();
    void unsupportedBuffersAreIgnored();
};

void TestMensuraDsp::identity()
{
    MensuraDsp dsp;
    QCOMPARE(dsp.id(), u"fooyin.dsp.mensura"_s);
    QCOMPARE(dsp.name(), u"Mensura"_s);
    QVERIFY(dsp.loadSettings({}));
    QVERIFY(dsp.saveSettings().isEmpty());
}

void TestMensuraDsp::disabledLeavesAudioUntouched()
{
    SharedState state; // default: disabled
    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 2});

    std::vector<double> input(static_cast<size_t>(Rate) * 2);
    for(size_t i = 0; i < input.size(); ++i) {
        input[i] = 0.7 * std::sin(static_cast<double>(i) * 0.01);
    }
    QCOMPARE(run(dsp, input, 2, 0), input);
}

void TestMensuraDsp::clickStartsOnExactFrame()
{
    SharedState state;
    MensuraParams params = enabledParams();
    params.phaseNs       = 1'010'000; // 48.48 frames -> beat 0 at frame 48
    state.publish(params);

    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 2});
    const auto out = run(dsp, std::vector<double>(static_cast<size_t>(Rate) * 2, 0.0), 2, 0);

    const ClickSynth synth{Rate};
    const auto accent = synth.click(ClickSound::Click, true);
    const auto normal = synth.click(ClickSound::Click, false);

    QVERIFY(isSilent(out, 2, 0, 48));
    QVERIFY(hasClickAt(out, 2, 48, accent, 0.5));
    QVERIFY(isSilent(out, 2, 48 + accent.size(), 24048));
    QVERIFY(hasClickAt(out, 2, 24048, normal, 0.5)); // 501.01 ms = frame 24048.48
    QVERIFY(isSilent(out, 2, 24048 + normal.size(), Rate));
}

void TestMensuraDsp::tailContinuesAcrossMsQuantisedBuffers()
{
    constexpr int Rate441 = 44100;
    SharedState state;
    state.publish(enabledParams()); // beats at frames 0, 22050, 44100, 66150 (22050 % 1024 = 546)

    const std::vector<double> input(static_cast<size_t>(Rate441) * 2 * 2, 0.0); // 2 s stereo

    MensuraDsp whole{state};
    whole.prepare(AudioFormat{SampleFormat::F64, Rate441, 2});
    const auto reference = run(whole, input, 2, 0, Rate441);

    MensuraDsp chunked{state};
    chunked.prepare(AudioFormat{SampleFormat::F64, Rate441, 2});
    const auto output = runChunked(chunked, input, 2, Rate441, {1024}, msQuantisedStartNs);

    QCOMPARE(output, reference);
}

void TestMensuraDsp::tinyAndEmptyBuffersMatchOneBigBuffer()
{
    SharedState state;
    state.publish(enabledParams());

    const std::vector<double> input(static_cast<size_t>(Rate) * 2, 0.0); // 2 s mono

    MensuraDsp whole{state};
    whole.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    const auto reference = run(whole, input, 1, 0);

    MensuraDsp chunked{state};
    chunked.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    const auto output = runChunked(chunked, input, 1, Rate, {0, 1, 7, 0, 64, 3, 480, 1}, exactStartNs);

    QCOMPARE(output, reference);
}

void TestMensuraDsp::seekDropsTail()
{
    SharedState state;
    state.publish(enabledParams());

    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 2});

    // Ends 10 frames into the click of beat 1 (frame 24000)
    const auto first = run(dsp, std::vector<double>(24010 * 2, 0.0), 2, 0);
    const ClickSynth synth{Rate};
    const auto normal = synth.click(ClickSound::Click, false);
    for(size_t k = 0; k < 10; ++k) {
        QCOMPARE(first[(24000 + k) * 2], normal[k] * 0.5);
    }

    // Seek to 10.1 s: next beat is at 10.5 s, far beyond this buffer
    const auto second = run(dsp, std::vector<double>(1000 * 2, 0.0), 2, 10'100'000'000);
    QVERIFY(isSilent(second, 2, 0, 1000));
}

void TestMensuraDsp::resetDropsTail()
{
    SharedState state;
    state.publish(enabledParams());

    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    run(dsp, std::vector<double>(24010, 0.0), 1, 0);

    dsp.reset();
    // Continues exactly where the previous buffer ended; the tail must not resume
    const auto next = run(dsp, std::vector<double>(1000, 0.0), 1, exactStartNs(24010, Rate));
    QVERIFY(isSilent(next, 1, 0, 1000));
}

void TestMensuraDsp::accentOnlyOnDownbeat()
{
    SharedState state;
    state.publish(enabledParams());

    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    const auto out = run(dsp, std::vector<double>(static_cast<size_t>(Rate) * 2, 0.0), 1, 0);

    const ClickSynth synth{Rate};
    QVERIFY(hasClickAt(out, 1, 0, synth.click(ClickSound::Click, true), 0.5));
    QVERIFY(hasClickAt(out, 1, 24000, synth.click(ClickSound::Click, false), 0.5));
    QVERIFY(hasClickAt(out, 1, 48000, synth.click(ClickSound::Click, false), 0.5));
    QVERIFY(hasClickAt(out, 1, 72000, synth.click(ClickSound::Click, false), 0.5));
}

void TestMensuraDsp::singleBeatBarHasNoAccent()
{
    SharedState state;
    MensuraParams params = enabledParams();
    params.beatsPerBar   = 1;
    params.sound         = ClickSound::Wood;
    state.publish(params);

    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    const auto out = run(dsp, std::vector<double>(Rate, 0.0), 1, 0);

    const ClickSynth synth{Rate};
    QVERIFY(hasClickAt(out, 1, 0, synth.click(ClickSound::Wood, false), 0.5));
    QVERIFY(hasClickAt(out, 1, 24000, synth.click(ClickSound::Wood, false), 0.5));
}

void TestMensuraDsp::heartbeatUpdatedOnProcess()
{
    SharedState state;
    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 2});
    QCOMPARE(state.lastHeartbeatNs(), int64_t{0});

    const int64_t before = SharedState::nowNs();
    run(dsp, std::vector<double>(64, 0.0), 2, 0);
    QVERIFY(state.lastHeartbeatNs() >= before);
}

void TestMensuraDsp::mixesIntoEveryChannel()
{
    SharedState state;
    state.publish(enabledParams());
    const ClickSynth synth{Rate};

    for(const int channels : {1, 2, 6}) {
        MensuraDsp dsp{state};
        dsp.prepare(AudioFormat{SampleFormat::F64, Rate, channels});
        const auto out = run(dsp, std::vector<double>(4000 * static_cast<size_t>(channels), 0.0), channels, 0);
        QVERIFY(hasClickAt(out, channels, 0, synth.click(ClickSound::Click, true), 0.5));
    }
}

void TestMensuraDsp::softClipBoundsOutput()
{
    SharedState state;
    MensuraParams params = enabledParams();
    params.gain          = 1.0;
    state.publish(params);

    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    const auto out = run(dsp, std::vector<double>(4000, 0.8), 1, 0);

    const size_t clickFrames = ClickSynth{Rate}.click(ClickSound::Click, true).size();
    bool reachedKnee{false};
    for(size_t i = 0; i < clickFrames; ++i) {
        QVERIFY(std::abs(out[i]) < 1.0);
        reachedKnee = reachedKnee || out[i] > 0.9;
    }
    QVERIFY(reachedKnee);
    for(size_t i = clickFrames; i < out.size(); ++i) {
        QCOMPARE(out[i], 0.8); // untouched after the click
    }
}

void TestMensuraDsp::sampleRateChangeWithoutPrepare()
{
    constexpr int Rate441 = 44100;
    SharedState state;
    state.publish(enabledParams());

    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    run(dsp, std::vector<double>(1000, 0.0), 1, 0);

    // Same track position, new rate, no prepare(): beat 1 is at 0.5 s = frame 22050
    const auto out = run(dsp, std::vector<double>(Rate441, 0.0), 1, 0, Rate441);
    const ClickSynth synth{Rate441};
    QVERIFY(hasClickAt(out, 1, 0, synth.click(ClickSound::Click, true), 0.5));
    QVERIFY(hasClickAt(out, 1, 22050, synth.click(ClickSound::Click, false), 0.5));
}

void TestMensuraDsp::twoInstancesKeepSeparateTimelines()
{
    SharedState state;
    state.publish(enabledParams());

    const std::vector<double> input(static_cast<size_t>(Rate) * 2 * 2, 0.0); // 2 s stereo
    const std::vector<int> chunks{512};
    constexpr uint64_t SecondTrackNs = 30'250'000'000; // 30.25 s: off-beat start

    const auto makeNode = [&state] {
        auto dsp = std::make_unique<MensuraDsp>(state);
        dsp->prepare(AudioFormat{SampleFormat::F64, Rate, 2});
        return dsp;
    };

    auto aloneA = makeNode();
    const auto referenceA = runChunked(*aloneA, input, 2, Rate, chunks, exactStartNs);
    auto aloneB = makeNode();
    const auto referenceB = runChunked(*aloneB, input, 2, Rate, chunks, exactStartNs, SecondTrackNs);

    auto nodeA = makeNode();
    auto nodeB = makeNode();
    std::vector<double> outA;
    std::vector<double> outB;
    const size_t totalFrames = input.size() / 2;
    for(size_t offset = 0; offset < totalFrames; offset += 512) {
        const size_t frames = std::min<size_t>(512, totalFrames - offset); // last chunk is partial
        const std::vector<double> chunk(input.begin() + static_cast<std::ptrdiff_t>(offset * 2),
                                        input.begin() + static_cast<std::ptrdiff_t>((offset + frames) * 2));
        const auto a = run(*nodeA, chunk, 2, exactStartNs(offset, Rate));
        const auto b = run(*nodeB, chunk, 2, SecondTrackNs + exactStartNs(offset, Rate));
        outA.insert(outA.end(), a.begin(), a.end());
        outB.insert(outB.end(), b.begin(), b.end());
    }

    QCOMPARE(outA, referenceA);
    QCOMPARE(outB, referenceB);
}

void TestMensuraDsp::disablingMidClickFinishesIt()
{
    SharedState state;
    state.publish(enabledParams());

    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    run(dsp, std::vector<double>(24010, 0.0), 1, 0); // click of beat 1 started at frame 24000

    MensuraParams disabled = enabledParams();
    disabled.enabled       = false;
    state.publish(disabled);

    const ClickSynth synth{Rate};
    const auto normal = synth.click(ClickSound::Click, false);

    const auto rest = run(dsp, std::vector<double>(2000, 0.0), 1, exactStartNs(24010, Rate));
    QVERIFY(hasClickAt(rest, 1, 0, normal, 0.5, 10)); // remaining samples of the same click
    QVERIFY(isSilent(rest, 1, normal.size() - 10, 2000));

    // Beat 2 (frame 48000) must not start
    const auto later = run(dsp, std::vector<double>(24000, 0.0), 1, exactStartNs(26010, Rate));
    QVERIFY(isSilent(later, 1, 0, 24000));
}

void TestMensuraDsp::unsupportedBuffersAreIgnored()
{
    SharedState state;
    state.publish(enabledParams());

    // fooyin's ProcessingBuffer always holds doubles and reports F64 whatever format it is
    // given, so a non-float buffer cannot reach the node; the F64 check is purely defensive.
    const ProcessingBuffer f32Buffer{std::vector<double>(4, 0.0), AudioFormat{SampleFormat::F32, Rate, 1}, 0};
    QCOMPARE(f32Buffer.format().sampleFormat(), SampleFormat::F64);

    // A buffer without a sample rate passes untouched
    MensuraDsp dsp{state};
    dsp.prepare(AudioFormat{SampleFormat::F64, Rate, 1});
    const std::vector<double> input(4000, 0.25);
    QCOMPARE(run(dsp, input, 1, 0, 0), input);
}

QTEST_GUILESS_MAIN(TestMensuraDsp)
#include "tst_mensuradsp.moc"
