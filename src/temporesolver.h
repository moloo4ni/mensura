#pragma once

#include <QString>

#include <cstdint>
#include <optional>

namespace Fooyin {
class Track;
}

namespace Fooyin::Mensura {
enum class BpmMode : uint8_t
{
    Auto = 0, // tag if present, otherwise the manual value
    Manual,
};

enum class BpmSource : uint8_t
{
    Tag = 0,
    Manual,
};

struct ResolvedTempo
{
    double bpm;
    BpmSource source;
};

//! First number in the text; no number or a value <= 0 means "no tag". Clamped to 20-300.
[[nodiscard]] std::optional<double> parseBpmTag(const QString& text);
//! First valid value of the track's BPM tag.
[[nodiscard]] std::optional<double> trackBpm(const Track& track);
[[nodiscard]] ResolvedTempo resolveTempo(BpmMode mode, std::optional<double> tagBpm, double manualBpm);
} // namespace Fooyin::Mensura
