#include "sharedstate.h"

#include <chrono>

namespace Fooyin::Mensura {
SharedState& SharedState::instance()
{
    static SharedState state;
    return state;
}

void SharedState::publish(const MensuraParams& params)
{
    constexpr auto Relaxed = std::memory_order_relaxed;

    const uint64_t seq = m_seq.load(Relaxed);
    m_seq.store(seq + 1, Relaxed); // odd: write in progress
    std::atomic_thread_fence(std::memory_order_release);

    m_enabled.store(params.enabled, Relaxed);
    m_bpm.store(params.bpm, Relaxed);
    m_beatsPerBar.store(params.beatsPerBar, Relaxed);
    m_sound.store(static_cast<uint8_t>(params.sound), Relaxed);
    m_gain.store(params.gain, Relaxed);
    m_phaseNs.store(params.phaseNs, Relaxed);

    m_seq.store(seq + 2, std::memory_order_release);
}

bool SharedState::tryRead(MensuraParams& out) const
{
    constexpr auto Relaxed = std::memory_order_relaxed;

    for(int attempt = 0; attempt < MaxReadAttempts; ++attempt) {
        const uint64_t before = m_seq.load(std::memory_order_acquire);
        if(before & 1U) {
            continue;
        }

        MensuraParams params;
        params.enabled     = m_enabled.load(Relaxed);
        params.bpm         = m_bpm.load(Relaxed);
        params.beatsPerBar = m_beatsPerBar.load(Relaxed);
        const uint8_t sound = m_sound.load(Relaxed);
        params.sound   = sound < ClickSoundCount ? static_cast<ClickSound>(sound) : ClickSound::Click;
        params.gain    = m_gain.load(Relaxed);
        params.phaseNs = m_phaseNs.load(Relaxed);

        std::atomic_thread_fence(std::memory_order_acquire);
        if(m_seq.load(Relaxed) == before) {
            out = params;
            return true;
        }
    }
    return false;
}

uint64_t SharedState::revision() const
{
    return m_seq.load(std::memory_order_acquire) / 2;
}

void SharedState::heartbeat(int64_t nowNs)
{
    m_heartbeatNs.store(nowNs, std::memory_order_relaxed);
}

int64_t SharedState::lastHeartbeatNs() const
{
    return m_heartbeatNs.load(std::memory_order_relaxed);
}

int64_t SharedState::nowNs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}
} // namespace Fooyin::Mensura
