#pragma once

#include "mensuraconfig.h"
#include "positionclock.h"
#include "taptempo.h"
#include "temporesolver.h"

#include <QObject>

#include <optional>

namespace Fooyin::Mensura {
class SharedState;

/*!
 * Owns the user config and playback-derived state (tag BPM, tap phase, position)
 * and publishes the effective parameters to the DSP node through SharedState.
 * Lives in the GUI thread; time is passed in explicitly as SharedState::nowNs() values.
 */
class MensuraController : public QObject
{
    Q_OBJECT

public:
    static constexpr int64_t HeartbeatTimeoutNs = 1'000'000'000;

    explicit MensuraController(SharedState& state, QObject* parent = nullptr);

    //! Replaces the config without emitting configChanged (used when loading settings).
    void loadConfig(const MensuraConfig& config);
    [[nodiscard]] const MensuraConfig& config() const;

    void setEnabled(bool enabled);
    void setMode(BpmMode mode);
    //! Also switches the mode to Manual.
    void setManualBpm(double bpm);
    void setBeatsPerBar(int beats);
    void setSound(ClickSound sound);
    void setAccent(AccentMode accent);
    void setVolumeDb(double db);
    void setPhaseOffsetMs(int offsetMs);

    //! Clears the tap phase, the tap series and the phase offset.
    void resetPhase();
    void tap(int64_t nowNs);

    void handleTrackChanged(std::optional<double> tagBpm, int64_t nowNs);
    void handleTrackUpdated(std::optional<double> tagBpm);
    void handlePlayStateChanged(bool playing, int64_t nowNs);
    void handlePosition(uint64_t positionMs, int64_t nowNs);
    void handleSeek(uint64_t positionMs, int64_t nowNs);
    void checkHeartbeat(int64_t nowNs);

    [[nodiscard]] MensuraParams params() const;
    [[nodiscard]] int64_t phaseNs() const;
    [[nodiscard]] ResolvedTempo tempo() const;
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] double positionMs(int64_t nowNs) const;
    [[nodiscard]] bool nodeMissing() const;

signals:
    //! Anything shown in the window may have changed.
    void stateChanged();
    //! The persistent config changed and should be saved.
    void configChanged();
    void nodeMissingChanged(bool missing);

private:
    bool updateConfig(const MensuraConfig& next);
    void publish();

    SharedState& m_state;
    MensuraConfig m_config;
    std::optional<double> m_tagBpm;
    TapTempo m_tap;
    PositionClock m_clock;
    int64_t m_tapPhaseNs{0};
    int64_t m_playStartNs{0};
    bool m_nodeMissing{false};
};
} // namespace Fooyin::Mensura
