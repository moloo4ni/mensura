#include "clicksynth.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace Fooyin::Mensura {
namespace {
constexpr double Pi        = std::numbers::pi;
constexpr double AttackMs  = 1.0;
constexpr double ReleaseMs = 5.0;
// Components above this fraction of the sample rate are dropped (sines) or clamped (noise filter)
constexpr double MaxFrequencyRatio = 0.45;

struct Partial
{
    double hz;
    double amplitude;
    double decayMs; // 0 = sustained
};

// Band-passed noise burst: the "strike" that makes a click cut through a dense mix
struct Noise
{
    double amplitude;
    double centreHz;
    double decayMs;
};

struct Voice
{
    double lengthMs;
    std::array<Partial, 3> partials; // amplitude 0 = unused
    Noise noise;                     // amplitude 0 = none
    double drive;                    // tanh saturation: raises loudness at the same peak; 0 = clean
};

// Indexed by ClickSound. Decays are long enough to carry energy; peaks are normalised later.
constexpr std::array<Voice, ClickSoundCount> Voices{{
    // Click: bright digital tick
    {.lengthMs = 40.0,
     .partials = {{{2000.0, 1.0, 20.0}, {5200.0, 0.5, 8.0}, {}}},
     .noise    = {.amplitude = 0.5, .centreHz = 6000.0, .decayMs = 3.0},
     .drive    = 3.0},
    // Wood: woodblock
    {.lengthMs = 60.0,
     .partials = {{{900.0, 1.0, 30.0}, {2400.0, 0.5, 15.0}, {3700.0, 0.25, 8.0}}},
     .noise    = {.amplitude = 0.4, .centreHz = 2500.0, .decayMs = 3.0},
     .drive    = 3.0},
    // Beep: sine with a little second harmonic for presence
    {.lengthMs = 60.0, .partials = {{{1000.0, 1.0, 0.0}, {2000.0, 0.2, 0.0}, {}}}, .noise = {}, .drive = 0.0},
    // Mechanical: pendulum metronome tick, mostly strike
    {.lengthMs = 45.0,
     .partials = {{{1600.0, 0.6, 20.0}, {4100.0, 0.4, 10.0}, {7300.0, 0.2, 4.0}}},
     .noise    = {.amplitude = 1.0, .centreHz = 3500.0, .decayMs = 6.0},
     .drive    = 4.0},
    // Clave: ringing hardwood tone
    {.lengthMs = 60.0,
     .partials = {{{2500.0, 1.0, 35.0}, {6000.0, 0.15, 10.0}, {}}},
     .noise    = {.amplitude = 0.3, .centreHz = 5000.0, .decayMs = 2.0},
     .drive    = 2.0},
}};

size_t msToFrames(double ms, int sampleRate)
{
    return static_cast<size_t>(std::lround(ms * sampleRate / 1000.0));
}

double decay(double t, double decayMs)
{
    return decayMs > 0.0 ? std::exp(-t * 1000.0 / decayMs) : 1.0;
}

// Deterministic white noise in [-1, 1], so every synth instance produces identical clicks
class NoiseSource
{
public:
    double next()
    {
        m_state = m_state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(m_state >> 11) / static_cast<double>(1ULL << 52) - 1.0;
    }

private:
    uint64_t m_state{0x9E3779B97F4A7C15ULL};
};

// RBJ band-pass biquad (constant 0 dB peak gain)
class BandPass
{
public:
    BandPass(double centreHz, int sampleRate)
    {
        constexpr double Q = 2.0;
        const double w0    = 2.0 * Pi * centreHz / sampleRate;
        const double alpha = std::sin(w0) / (2.0 * Q);
        const double a0    = 1.0 + alpha;
        m_b0               = alpha / a0;
        m_b2               = -alpha / a0;
        m_a1               = -2.0 * std::cos(w0) / a0;
        m_a2               = (1.0 - alpha) / a0;
    }

    double process(double x)
    {
        const double y = m_b0 * x + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
        m_x2           = m_x1;
        m_x1           = x;
        m_y2           = m_y1;
        m_y1           = y;
        return y;
    }

private:
    double m_b0, m_b2, m_a1, m_a2;
    double m_x1{0.0}, m_x2{0.0}, m_y1{0.0}, m_y2{0.0};
};

std::vector<double> synthesise(const Voice& voice, int sampleRate, bool accent)
{
    const size_t frames  = msToFrames(voice.lengthMs, sampleRate);
    const size_t attack  = std::max<size_t>(1, msToFrames(AttackMs, sampleRate));
    const size_t release = std::max<size_t>(2, msToFrames(ReleaseMs, sampleRate));
    const double pitch   = accent ? ClickSynth::AccentPitch : 1.0;
    const double maxHz   = MaxFrequencyRatio * sampleRate;

    NoiseSource noise;
    BandPass filter{std::min(voice.noise.centreHz * pitch, maxHz), sampleRate};

    std::vector<double> samples(frames);
    double peak{0.0};

    for(size_t i = 0; i < frames; ++i) {
        const double t = static_cast<double>(i) / sampleRate;

        double sample{0.0};
        for(const Partial& partial : voice.partials) {
            const double hz = partial.hz * pitch;
            if(partial.amplitude > 0.0 && hz < maxHz) {
                sample += partial.amplitude * std::sin(2.0 * Pi * hz * t) * decay(t, partial.decayMs);
            }
        }
        if(voice.noise.amplitude > 0.0) {
            sample += voice.noise.amplitude * filter.process(noise.next()) * decay(t, voice.noise.decayMs);
        }
        if(voice.drive > 0.0) {
            sample = std::tanh(voice.drive * sample);
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
