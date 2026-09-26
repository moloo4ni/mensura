#pragma once

#include <cstdint>

namespace Fooyin::Mensura {
//! Audible track position: the last reported position advanced by monotonic time while playing.
class PositionClock
{
public:
    void setPosition(uint64_t positionMs, int64_t nowNs);
    void setPlaying(bool playing, int64_t nowNs);

    [[nodiscard]] double positionMs(int64_t nowNs) const;
    [[nodiscard]] bool isPlaying() const;

private:
    double m_baseMs{0.0};
    int64_t m_baseNs{0};
    bool m_playing{false};
};
} // namespace Fooyin::Mensura
