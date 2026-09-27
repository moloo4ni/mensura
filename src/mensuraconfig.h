#pragma once

#include "mensuraparams.h"
#include "temporesolver.h"

#include <QVariantMap>

namespace Fooyin::Mensura {
//! User settings persisted between sessions.
struct MensuraConfig
{
    static constexpr double MinVolumeDb     = -40.0;
    static constexpr double MaxVolumeDb     = 0.0;
    static constexpr double DefaultVolumeDb = -6.0;
    static constexpr int MaxPhaseOffsetMs   = 1000;
    static constexpr int Version            = 1;

    bool enabled{false};
    BpmMode mode{BpmMode::Auto};
    double manualBpm{DefaultBpm};
    int beatsPerBar{4};
    ClickSound sound{ClickSound::Click};
    AccentMode accent{AccentMode::Pitch};
    double volumeDb{DefaultVolumeDb};
    int phaseOffsetMs{0};

    [[nodiscard]] QVariantMap toMap() const;
    //! Every value is validated independently; anything unusable falls back to its default.
    [[nodiscard]] static MensuraConfig fromMap(const QVariantMap& map);

    bool operator==(const MensuraConfig&) const = default;
};

[[nodiscard]] double dbToGain(double db);
} // namespace Fooyin::Mensura
