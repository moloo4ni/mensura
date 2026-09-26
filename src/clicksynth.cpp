#include "clicksynth.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Fooyin::Mensura {
namespace {
constexpr double Pi        = std::numbers::pi;
constexpr double AttackMs  = 1.0;
constexpr double ReleaseMs = 5.0;

struct Voice
{
    double lengthMs;
    double baseHz;
    double overtoneHz; // 0 = none
    double decayMs;    // 0 = no exponential decay
};

// Indexed by ClickSound
constexpr std::array<Voice, ClickSoundCount> Voices{{
    {.lengthMs = 30.0, .baseHz = 2000.0, .overtoneHz = 5200.0, .decayMs = 4.0},  // Click
    {.lengthMs = 50.0, .baseHz = 900.0, .overtoneHz = 2400.0, .decayMs = 10.0},  // Wood
    {.lengthMs = 60.0, .baseHz = 1000.0, .overtoneHz = 0.0, .decayMs = 0.0},     // Beep
}};

size_t msToFrames(double ms, int sampleRate)
{
    return static_cast<size_t>(std::lround(ms * sampleRate / 1000.0));
}

std::vector<double> synthesise(const Voice& voice, int sampleRate, bool accent)
{
    const size_t frames  = msToFrames(voice.lengthMs, sampleRate);
    const size_t attack  = std::max<size_t>(1, msToFrames(AttackMs, sampleRate));
    const size_t release = std::max<size_t>(2, msToFrames(ReleaseMs, sampleRate));
    const double pitch   = accent ? ClickSynth::AccentPitch : 1.0;

    std::vector<double> samples(frames);
    double peak{0.0};

    for(size_t i = 0; i < frames; ++i) {
        const double t = static_cast<double>(i) / sampleRate;

        double sample = std::sin(2.0 * Pi * voice.baseHz * pitch * t);
        if(voice.overtoneHz > 0.0) {
            sample += 0.5 * std::sin(2.0 * Pi * voice.overtoneHz * pitch * t);
        }
        if(voice.decayMs > 0.0) {
            sample *= std::exp(-t * 1000.0 / voice.decayMs);
        }

        // Raised-cosine fade in and out, so the first and last samples are exactly 0
        double envelope{1.0};
        if(i < attack) {
            envelope = 0.5 - 0.5 * std::cos(Pi * static_cast<double>(i) / static_cast<double>(attack));
        }
        const size_t fromEnd = frames - 1 - i;
        if(fromEnd < release) {
            envelope *= 0.5 - 0.5 * std::cos(Pi * static_cast<double>(fromEnd) / static_cast<double>(release - 1));
        }

        samples[i] = sample * envelope;
        peak       = std::max(peak, std::abs(samples[i]));
    }

    if(peak > 0.0) {
        const double scale = (accent ? ClickSynth::AccentPeak : ClickSynth::NormalPeak) / peak;
        for(double& sample : samples) {
            sample *= scale;
        }
    }

    return samples;
}

size_t slot(ClickSound sound, bool accent)
{
    auto index = static_cast<size_t>(sound);
    if(index >= static_cast<size_t>(ClickSoundCount)) {
        index = 0;
    }
    return index * 2 + (accent ? 1 : 0);
}
} // namespace

ClickSynth::ClickSynth(int sampleRate)
    : m_sampleRate{sampleRate}
{
    for(int sound = 0; sound < ClickSoundCount; ++sound) {
        for(const bool accent : {false, true}) {
            m_clicks[slot(static_cast<ClickSound>(sound), accent)]
                = synthesise(Voices[static_cast<size_t>(sound)], sampleRate, accent);
        }
    }
}

int ClickSynth::sampleRate() const
{
    return m_sampleRate;
}

std::span<const double> ClickSynth::click(ClickSound sound, bool accent) const
{
    return m_clicks[slot(sound, accent)];
}
} // namespace Fooyin::Mensura
