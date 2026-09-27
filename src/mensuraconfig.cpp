#include "mensuraconfig.h"

#include <algorithm>
#include <cmath>
#include <optional>

using namespace Qt::StringLiterals;

namespace Fooyin::Mensura {
namespace {
std::optional<double> number(const QVariantMap& map, const QString& key)
{
    const auto it = map.constFind(key);
    if(it == map.cend()) {
        return {};
    }
    bool ok{false};
    const double value = it->toDouble(&ok);
    if(!ok || !std::isfinite(value)) {
        return {};
    }
    return value;
}

//! Accepts a bool, the numbers 0/1, or the strings true/false/1/0 (case-insensitive, trimmed).
std::optional<bool> flag(const QVariantMap& map, const QString& key)
{
    const auto it = map.constFind(key);
    if(it == map.cend()) {
        return {};
    }
    if(it->typeId() == QMetaType::Bool) {
        return it->toBool();
    }
    if(it->typeId() == QMetaType::QString) {
        const QString text = it->toString().trimmed();
        if(text.compare(u"true"_s, Qt::CaseInsensitive) == 0 || text == u"1"_s) {
            return true;
        }
        if(text.compare(u"false"_s, Qt::CaseInsensitive) == 0 || text == u"0"_s) {
            return false;
        }
        return {};
    }
    if(const auto value = number(map, key); value && (*value == 0.0 || *value == 1.0)) {
        return *value == 1.0;
    }
    return {};
}

int clampedInt(double value, int min, int max)
{
    return static_cast<int>(std::lround(std::clamp(value, static_cast<double>(min), static_cast<double>(max))));
}

bool isIndex(double value, int count)
{
    return value >= 0.0 && value < count && value == std::floor(value);
}
} // namespace

QVariantMap MensuraConfig::toMap() const
{
    return {
        {u"Version"_s, Version},
        {u"Enabled"_s, enabled},
        {u"Mode"_s, static_cast<int>(mode)},
        {u"ManualBpm"_s, manualBpm},
        {u"BeatsPerBar"_s, beatsPerBar},
        {u"Sound"_s, static_cast<int>(sound)},
        {u"Accent"_s, static_cast<int>(accent)},
        {u"VolumeDb"_s, volumeDb},
        {u"PhaseOffsetMs"_s, phaseOffsetMs},
    };
}

MensuraConfig MensuraConfig::fromMap(const QVariantMap& map)
{
    MensuraConfig config;

    if(const auto enabled = flag(map, u"Enabled"_s)) {
        config.enabled = *enabled;
    }
    if(const auto mode = number(map, u"Mode"_s); mode && isIndex(*mode, BpmModeCount)) {
        config.mode = static_cast<BpmMode>(static_cast<int>(*mode));
    }
    if(const auto bpm = number(map, u"ManualBpm"_s)) {
        config.manualBpm = clampBpm(*bpm);
    }
    if(const auto beats = number(map, u"BeatsPerBar"_s)) {
        config.beatsPerBar = clampedInt(*beats, MinBeatsPerBar, MaxBeatsPerBar);
    }
    if(const auto sound = number(map, u"Sound"_s); sound && isIndex(*sound, ClickSoundCount)) {
        config.sound = static_cast<ClickSound>(static_cast<int>(*sound));
    }
    if(const auto accent = number(map, u"Accent"_s); accent && isIndex(*accent, AccentModeCount)) {
        config.accent = static_cast<AccentMode>(static_cast<int>(*accent));
    }
    if(const auto volume = number(map, u"VolumeDb"_s)) {
        config.volumeDb = std::clamp(*volume, MinVolumeDb, MaxVolumeDb);
    }
    if(const auto offset = number(map, u"PhaseOffsetMs"_s)) {
        config.phaseOffsetMs = clampedInt(*offset, -MaxPhaseOffsetMs, MaxPhaseOffsetMs);
    }

    return config;
}

double dbToGain(double db)
{
    return std::pow(10.0, db / 20.0);
}
} // namespace Fooyin::Mensura
