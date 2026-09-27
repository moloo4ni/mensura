#include "clicksynth.h"

#include <QTest>

#include <algorithm>
#include <cmath>

using namespace Fooyin::Mensura;

namespace {
constexpr std::array AllSounds{ClickSound::Click, ClickSound::Wood, ClickSound::Beep, ClickSound::Mechanical};

double peak(std::span<const double> samples)
{
    double result{0.0};
    for(const double s : samples) {
        result = std::max(result, std::abs(s));
    }
    return result;
}

int signChanges(std::span<const double> samples)
{
    int changes{0};
    for(size_t i = 1; i < samples.size(); ++i) {
        if((samples[i - 1] < 0.0) != (samples[i] < 0.0)) {
            ++changes;
        }
    }
    return changes;
}
} // namespace

class TestClickSynth : public QObject
{
    Q_OBJECT

private slots:
    void lengthsMatchVoices();
    void lengthScalesWithSampleRate();
    void edgesAreSilent();
    void peaksAreNormalised();
    void accentIsHigherPitched();
    void samplesAreFinite();
    void soundsAreComparablyLoud();
    void synthesisIsDeterministic();
    void outOfRangeSoundFallsBackToClick();
};

void TestClickSynth::lengthsMatchVoices()
{
    const ClickSynth synth{48000};
    QCOMPARE(synth.sampleRate(), 48000);
    for(const bool accent : {false, true}) {
        QCOMPARE(synth.click(ClickSound::Click, accent).size(), size_t{1920});
        QCOMPARE(synth.click(ClickSound::Wood, accent).size(), size_t{2880});
        QCOMPARE(synth.click(ClickSound::Beep, accent).size(), size_t{2880});
        QCOMPARE(synth.click(ClickSound::Mechanical, accent).size(), size_t{2160});
    }
}

void TestClickSynth::lengthScalesWithSampleRate()
{
    const ClickSynth synth{44100};
    QCOMPARE(synth.click(ClickSound::Click, false).size(), size_t{1764});
}

void TestClickSynth::edgesAreSilent()
{
    const ClickSynth synth{48000};
    for(const auto sound : AllSounds) {
        for(const bool accent : {false, true}) {
            const auto click = synth.click(sound, accent);
            QCOMPARE(click.front(), 0.0);
            QCOMPARE(click.back(), 0.0);
        }
    }
}

void TestClickSynth::peaksAreNormalised()
{
    const ClickSynth synth{48000};
    for(const auto sound : AllSounds) {
        QVERIFY(std::abs(peak(synth.click(sound, false)) - ClickSynth::Peak) < 1e-9);
        QVERIFY(std::abs(peak(synth.click(sound, true)) - ClickSynth::Peak) < 1e-9); // same level as a normal beat
    }
}

void TestClickSynth::accentIsHigherPitched()
{
    const ClickSynth synth{48000};
    for(const auto sound : AllSounds) {
        QVERIFY(signChanges(synth.click(sound, true)) > signChanges(synth.click(sound, false)));
    }
}

void TestClickSynth::samplesAreFinite()
{
    for(const int rate : {8000, 44100, 48000, 96000, 192000}) {
        const ClickSynth synth{rate};
        for(const auto sound : AllSounds) {
            for(const bool accent : {false, true}) {
                const auto click = synth.click(sound, accent);
                QVERIFY(std::ranges::all_of(click, [](double s) { return std::isfinite(s); }));
            }
        }
    }
}

void TestClickSynth::soundsAreComparablyLoud()
{
    // Energy over a 50 ms window: short clicks with the same peak used to be 15 dB quieter than Beep
    const ClickSynth synth{48000};
    const double window = 0.050 * synth.sampleRate();
    for(const auto sound : AllSounds) {
        double energy{0.0};
        for(const double s : synth.click(sound, false)) {
            energy += s * s;
        }
        const double rmsDb = 20.0 * std::log10(std::sqrt(energy / window));
        QVERIFY2(rmsDb > -13.0, qPrintable(QString::number(rmsDb)));
    }
}

void TestClickSynth::synthesisIsDeterministic()
{
    const ClickSynth first{48000};
    const ClickSynth second{48000};
    for(const auto sound : AllSounds) {
        QVERIFY(std::ranges::equal(first.click(sound, true), second.click(sound, true)));
    }
}

void TestClickSynth::outOfRangeSoundFallsBackToClick()
{
    const ClickSynth synth{48000};
    QCOMPARE(synth.click(static_cast<ClickSound>(7), false).data(), synth.click(ClickSound::Click, false).data());
}

QTEST_APPLESS_MAIN(TestClickSynth)
#include "tst_clicksynth.moc"
