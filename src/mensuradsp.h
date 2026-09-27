#pragma once

#include "clicksynth.h"
#include "mensuraparams.h"

#include <core/engine/dsp/dspnode.h>

#include <optional>
#include <span>

namespace Fooyin {
class ProcessingBuffer;
}

namespace Fooyin::Mensura {
class SharedState;

/*!
 * Mixes metronome clicks into the track. Parameters come from SharedState;
 * the node keeps its own frame-accurate timeline because fooyin quantises
 * per-track buffer start times to whole milliseconds.
 */
class MensuraDsp : public DspNode
{
public:
    static constexpr auto Id = "fooyin.dsp.mensura";
    //! Re-anchor the timeline when a buffer starts further than this from where we expect it.
    static constexpr int64_t ResyncThresholdNs = 5'000'000;
    //! Level of the other beats with AccentMode::PitchAndVolume: the downbeat stands out by 6 dB.
    static constexpr double UnaccentedLevel = 0.5;

    MensuraDsp();
    explicit MensuraDsp(SharedState& state);

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString id() const override;

    void prepare(const AudioFormat& format) override;
    void process(ProcessingBufferList& chunks) override;
    void reset() override;
    void flush(ProcessingBufferList& chunks, FlushMode mode) override;

    [[nodiscard]] QByteArray saveSettings() const override;
    bool loadSettings(const QByteArray& preset) override;

private:
    void processBuffer(ProcessingBuffer& buffer);
    void ensureSampleRate(int sampleRate);
    void syncTimeline(uint64_t startNs);
    void resetTimeline();
    void mixTail(std::span<double> samples, int channels, int fromFrame, int toFrame);

    SharedState& m_state;
    MensuraParams m_params;

    std::optional<ClickSynth> m_synth;
    int m_sampleRate{0};
    double m_nsPerFrame{0.0};

    bool m_anchored{false};
    uint64_t m_anchorNs{0};
    int64_t m_frameCursor{0}; // frames processed since the anchor

    std::span<const double> m_tail; // click currently sounding
    size_t m_tailPos{0};
    double m_tailLevel{1.0}; // fixed for the whole click, so it never jumps mid-click
};
} // namespace Fooyin::Mensura
