#include "mensuracontroller.h"

#include "sharedstate.h"

#include <algorithm>
#include <cmath>

namespace Fooyin::Mensura {
MensuraController::MensuraController(SharedState& state, QObject* parent)
    : QObject{parent}
    , m_state{state}
{
    publish();
}

void MensuraController::loadConfig(const MensuraConfig& config)
{
    m_config = MensuraConfig::fromMap(config.toMap());
    publish();
    emit stateChanged();
}

const MensuraConfig& MensuraController::config() const
{
    return m_config;
}

void MensuraController::setEnabled(bool enabled)
{
    MensuraConfig next = m_config;
    next.enabled       = enabled;
    updateConfig(next);
}

void MensuraController::setMode(BpmMode mode)
{
    if(static_cast<int>(mode) >= BpmModeCount) {
        return;
    }
    MensuraConfig next = m_config;
    next.mode          = mode;
    updateConfig(next);
}

void MensuraController::setManualBpm(double bpm)
{
    MensuraConfig next = m_config;
    next.manualBpm     = clampBpm(bpm);
    next.mode          = BpmMode::Manual;
    updateConfig(next);
}

void MensuraController::setBeatsPerBar(int beats)
{
    MensuraConfig next = m_config;
    next.beatsPerBar   = std::clamp(beats, MinBeatsPerBar, MaxBeatsPerBar);
    updateConfig(next);
}

void MensuraController::setSound(ClickSound sound)
{
    const auto index = static_cast<int>(sound);
    if(index < 0 || index >= ClickSoundCount) {
        return;
    }
    MensuraConfig next = m_config;
    next.sound         = sound;
    updateConfig(next);
}

void MensuraController::setAccent(AccentMode accent)
{
    const auto index = static_cast<int>(accent);
    if(index < 0 || index >= AccentModeCount) {
        return;
    }
    MensuraConfig next = m_config;
    next.accent        = accent;
    updateConfig(next);
}

void MensuraController::setVolumeDb(double db)
{
    if(!std::isfinite(db)) {
        return;
    }
    MensuraConfig next = m_config;
    next.volumeDb      = std::clamp(db, MensuraConfig::MinVolumeDb, MensuraConfig::MaxVolumeDb);
    updateConfig(next);
}

void MensuraController::setPhaseOffsetMs(int offsetMs)
{
    MensuraConfig next  = m_config;
    next.phaseOffsetMs = std::clamp(offsetMs, -MensuraConfig::MaxPhaseOffsetMs, MensuraConfig::MaxPhaseOffsetMs);
    updateConfig(next);
}

void MensuraController::resetPhase()
{
    m_tapPhaseNs = 0;
    m_tap.reset();

    MensuraConfig next  = m_config;
    next.phaseOffsetMs = 0;
    if(!updateConfig(next)) {
        publish(); // the tap phase changed even if the config did not
        emit stateChanged();
    }
}

void MensuraController::tap(int64_t nowNs)
{
    if(!m_clock.isPlaying()) {
        return;
    }

    const double positionMs = m_clock.positionMs(nowNs);
    m_tap.tap(positionMs);

    bool configUpdated{false};
    if(tempo().source == BpmSource::Manual) {
        if(const auto bpm = m_tap.bpm(); bpm && *bpm != m_config.manualBpm) {
            m_config.manualBpm = *bpm;
            configUpdated      = true;
        }
    }

    // First tap of the series is beat 0, so the last tap is beat (count - 1)
    const double periodNs = 60e9 / tempo().bpm;
    m_tapPhaseNs          = std::llround(positionMs * 1e6 - (m_tap.tapCount() - 1) * periodNs);

    publish();
    if(configUpdated) {
        emit configChanged();
    }
    emit stateChanged();
}

void MensuraController::handleTrackChanged(std::optional<double> tagBpm, int64_t nowNs)
{
    m_tagBpm     = tagBpm;
    m_tapPhaseNs = 0;
    m_tap.reset();
    m_clock.setPosition(0, nowNs);
    m_playStartNs = nowNs; // the pipeline may be refilled: restart the heartbeat grace period

    MensuraConfig next  = m_config;
    next.phaseOffsetMs = 0;
    if(!updateConfig(next)) {
        publish();
        emit stateChanged();
    }
}

void MensuraController::handleTrackUpdated(std::optional<double> tagBpm)
{
    if(tagBpm == m_tagBpm) {
        return;
    }
    m_tagBpm = tagBpm;
    publish();
    emit stateChanged();
}

void MensuraController::handlePlayStateChanged(bool playing, int64_t nowNs)
{
    if(playing && !m_clock.isPlaying()) {
        m_playStartNs = nowNs;
    }
    m_clock.setPlaying(playing, nowNs);
    emit stateChanged();
}

void MensuraController::handlePosition(uint64_t positionMs, int64_t nowNs)
{
    m_clock.setPosition(positionMs, nowNs);
}

void MensuraController::handleSeek(uint64_t positionMs, int64_t nowNs)
{
    m_clock.setPosition(positionMs, nowNs);
    m_tap.reset();
    m_playStartNs = nowNs; // the pipeline may be refilled: restart the heartbeat grace period
}

void MensuraController::checkHeartbeat(int64_t nowNs)
{
    const int64_t since = std::max(m_state.lastHeartbeatNs(), m_playStartNs);
    const bool missing  = m_clock.isPlaying() && m_config.enabled && nowNs - since > HeartbeatTimeoutNs;
    if(missing != m_nodeMissing) {
        m_nodeMissing = missing;
        emit nodeMissingChanged(missing);
    }
}

MensuraParams MensuraController::params() const
{
    return {.enabled     = m_config.enabled,
            .bpm         = tempo().bpm,
            .beatsPerBar = m_config.beatsPerBar,
            .sound       = m_config.sound,
            .accent      = m_config.accent,
            .gain        = dbToGain(m_config.volumeDb),
            .phaseNs     = phaseNs()};
}

int64_t MensuraController::phaseNs() const
{
    return m_tapPhaseNs + static_cast<int64_t>(m_config.phaseOffsetMs) * 1'000'000;
}

ResolvedTempo MensuraController::tempo() const
{
    return resolveTempo(m_config.mode, m_tagBpm, m_config.manualBpm);
}

bool MensuraController::isPlaying() const
{
    return m_clock.isPlaying();
}

double MensuraController::positionMs(int64_t nowNs) const
{
    return m_clock.positionMs(nowNs);
}

bool MensuraController::nodeMissing() const
{
    return m_nodeMissing;
}

bool MensuraController::updateConfig(const MensuraConfig& next)
{
    if(next == m_config) {
        return false;
    }
    m_config = next;
    publish();
    emit configChanged();
    emit stateChanged();
    return true;
}

void MensuraController::publish()
{
    m_state.publish(params());
}
} // namespace Fooyin::Mensura
