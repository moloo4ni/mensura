#pragma once

#include "mensuraparams.h"

#include <atomic>
#include <cstdint>

namespace Fooyin::Mensura {
/*!
 * In-process channel from the controller (GUI thread, single writer) to DSP
 * nodes (audio thread). A seqlock over atomic fields: no locks, no allocations.
 */
class SharedState
{
public:
    static constexpr int MaxReadAttempts = 8;

    SharedState() = default;
    SharedState(const SharedState&)            = delete;
    SharedState& operator=(const SharedState&) = delete;

    static SharedState& instance();

    void publish(const MensuraParams& params);
    //! False if no consistent snapshot was obtained; out is left untouched then.
    bool tryRead(MensuraParams& out) const;
    //! Number of completed publishes.
    [[nodiscard]] uint64_t revision() const;

    void heartbeat(int64_t nowNs);
    [[nodiscard]] int64_t lastHeartbeatNs() const;

    [[nodiscard]] static int64_t nowNs();

private:
    std::atomic<uint64_t> m_seq{0};
    std::atomic<bool> m_enabled{false};
    std::atomic<double> m_bpm{DefaultBpm};
    std::atomic<int> m_beatsPerBar{4};
    std::atomic<uint8_t> m_sound{0};
    std::atomic<double> m_gain{0.5};
    std::atomic<int64_t> m_phaseNs{0};
    std::atomic<int64_t> m_heartbeatNs{0};

    static_assert(std::atomic<double>::is_always_lock_free);
    static_assert(std::atomic<int64_t>::is_always_lock_free);
};
} // namespace Fooyin::Mensura
