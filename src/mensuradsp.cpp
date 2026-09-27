#include "mensuradsp.h"

#include "beatgrid.h"
#include "sharedstate.h"

#include <core/engine/dsp/processingbuffer.h>
#include <core/engine/dsp/processingbufferlist.h>

#include <cmath>

using namespace Qt::StringLiterals;

namespace Fooyin::Mensura {
namespace {
constexpr double ClipKnee = 0.9;

double softClip(double sample)
{
    const double magnitude = std::abs(sample);
    if(magnitude <= ClipKnee) {
        return sample;
    }
    const double shaped = ClipKnee + (1.0 - ClipKnee) * std::tanh((magnitude - ClipKnee) / (1.0 - ClipKnee));
    return std::copysign(shaped, sample);
}
} // namespace

MensuraDsp::MensuraDsp()
    : MensuraDsp{SharedState::instance()}
{ }

MensuraDsp::MensuraDsp(SharedState& state)
    : m_state{state}
{ }

QString MensuraDsp::name() const
{
    return u"Mensura"_s;
}

QString MensuraDsp::id() const
{
    return QString::fromLatin1(Id);
}

void MensuraDsp::prepare(const AudioFormat& format)
{
    resetTimeline();
    ensureSampleRate(format.sampleRate());
}

void MensuraDsp::process(ProcessingBufferList& chunks)
{
    m_state.heartbeat(SharedState::nowNs());
    m_state.tryRead(m_params); // on contention keep the previous snapshot

    for(size_t i = 0; i < chunks.count(); ++i) {
        if(auto* buffer = chunks.item(i); buffer && buffer->isValid()) {
            processBuffer(*buffer);
        }
    }
}

void MensuraDsp::reset()
{
    resetTimeline();
}

void MensuraDsp::flush(ProcessingBufferList& /*chunks*/, FlushMode mode)
{
    if(mode == FlushMode::Flush) {
        resetTimeline();
    }
}

QByteArray MensuraDsp::saveSettings() const
{
    // All settings live in the controller (SharedState), not in DSP presets.
    return {};
}

bool MensuraDsp::loadSettings(const QByteArray& /*preset*/)
{
    return true;
}

void MensuraDsp::processBuffer(ProcessingBuffer& buffer)
{
    const AudioFormat format = buffer.format();
    const int channels       = format.channelCount();
    const int frames         = buffer.frameCount();
    if(format.sampleFormat() != SampleFormat::F64 || channels <= 0 || frames <= 0 || format.sampleRate() <= 0) {
        return;
    }

    ensureSampleRate(format.sampleRate());
    syncTimeline(buffer.startTimeNs());

    const std::span<double> samples = buffer.data();
    const int64_t firstFrame        = m_frameCursor;
    const int64_t endFrame          = firstFrame + frames;
    int cursor{0};

    if(m_params.enabled) {
        const BeatGrid grid{m_params.bpm, m_params.phaseNs, m_params.beatsPerBar};
        const auto anchor  = static_cast<double>(m_anchorNs);
        const auto frameOf = [&](int64_t beat) {
            return std::llround((grid.beatTimeNs(beat) - anchor) / m_nsPerFrame);
        };
        const auto firstTimeNs
            = static_cast<int64_t>(m_anchorNs) + std::llround(static_cast<double>(firstFrame) * m_nsPerFrame);

        for(int64_t beat = grid.beatAtOrBefore(firstTimeNs);; ++beat) {
            const int64_t beatFrame = frameOf(beat);
            if(beatFrame >= endFrame) {
                break;
            }
            if(beatFrame < firstFrame) {
                continue;
            }
            const auto local = static_cast<int>(beatFrame - firstFrame);
            mixTail(samples, channels, cursor, local);

            const bool hasBar   = m_params.beatsPerBar > 1;
            const bool downbeat = hasBar && grid.indexInBar(beat) == 0;
            const bool accent   = downbeat && m_params.accent != AccentMode::None;
            const bool lowered  = hasBar && !downbeat && m_params.accent == AccentMode::PitchAndVolume;
            m_tail              = m_synth->click(m_params.sound, accent);
            m_tailPos           = 0;
            m_tailLevel         = lowered ? UnaccentedLevel : 1.0;
            cursor            = local;
        }
    }

    mixTail(samples, channels, cursor, frames);
    m_frameCursor = endFrame;
}

void MensuraDsp::ensureSampleRate(int sampleRate)
{
    if(sampleRate <= 0 || sampleRate == m_sampleRate) {
        return;
    }
    resetTimeline(); // the tail points into the old synth's buffers
    m_synth.emplace(sampleRate);
    m_sampleRate = sampleRate;
    m_nsPerFrame = 1e9 / sampleRate;
}

void MensuraDsp::syncTimeline(uint64_t startNs)
{
    if(m_anchored) {
        const double expected
            = static_cast<double>(m_anchorNs) + static_cast<double>(m_frameCursor) * m_nsPerFrame;
        if(std::abs(static_cast<double>(startNs) - expected) <= static_cast<double>(ResyncThresholdNs)) {
            return;
        }
    }
    resetTimeline();
    m_anchored = true;
    m_anchorNs = startNs;
}

void MensuraDsp::resetTimeline()
{
    m_anchored    = false;
    m_anchorNs    = 0;
    m_frameCursor = 0;
    m_tail        = {};
    m_tailPos     = 0;
    m_tailLevel   = 1.0;
}

void MensuraDsp::mixTail(std::span<double> samples, int channels, int fromFrame, int toFrame)
{
    const double gain = m_params.gain * m_tailLevel;
    for(int frame = fromFrame; frame < toFrame && m_tailPos < m_tail.size(); ++frame, ++m_tailPos) {
        const double click = m_tail[m_tailPos] * gain;
        double* out        = samples.data() + static_cast<size_t>(frame) * static_cast<size_t>(channels);
        for(int ch = 0; ch < channels; ++ch) {
            out[ch] = softClip(out[ch] + click);
        }
    }
    if(m_tailPos >= m_tail.size()) {
        m_tail    = {};
        m_tailPos = 0;
    }
}
} // namespace Fooyin::Mensura
