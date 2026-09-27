#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Fooyin::Mensura {
inline constexpr double MinBpm     = 20.0;
inline constexpr double MaxBpm     = 300.0;
inline constexpr double DefaultBpm = 120.0;

inline constexpr int MinBeatsPerBar = 1;
inline constexpr int MaxBeatsPerBar = 16;

enum class ClickSound : uint8_t
{
    Click = 0,
    Wood,
    Beep,
    Mechanical,
    Clave,
};
inline constexpr int ClickSoundCount = 5;

//! How the first beat of a bar stands out.
enum class AccentMode : uint8_t
{
    None = 0,
    Pitch,
    PitchAndVolume,
};
inline constexpr int AccentModeCount = 3;

//! Non-finite values fall back to the default tempo.
[[nodiscard]] inline double clampBpm(double bpm)
{
    return std::isfinite(bpm) ? std::clamp(bpm, MinBpm, MaxBpm) : DefaultBpm;
}

//! Effective parameters consumed by the DSP node.
struct MensuraParams
{
    bool enabled{false};
    double bpm{DefaultBpm};
    int beatsPerBar{4};
    ClickSound sound{ClickSound::Click};
    AccentMode accent{AccentMode::Pitch};
    double gain{0.5}; // linear
    int64_t phaseNs{0};

    bool operator==(const MensuraParams&) const = default;
};
} // namespace Fooyin::Mensura
