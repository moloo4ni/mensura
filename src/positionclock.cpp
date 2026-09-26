#include "positionclock.h"

#include <algorithm>

namespace Fooyin::Mensura {
void PositionClock::setPosition(uint64_t positionMs, int64_t nowNs)
{
    m_baseMs = static_cast<double>(positionMs);
    m_baseNs = nowNs;
}

void PositionClock::setPlaying(bool playing, int64_t nowNs)
{
    if(playing == m_playing) {
        return;
    }
    m_baseMs  = positionMs(nowNs);
    m_baseNs  = nowNs;
    m_playing = playing;
}

double PositionClock::positionMs(int64_t nowNs) const
{
    if(!m_playing) {
        return m_baseMs;
    }
    return m_baseMs + static_cast<double>(std::max<int64_t>(0, nowNs - m_baseNs)) / 1e6;
}

bool PositionClock::isPlaying() const
{
    return m_playing;
}
} // namespace Fooyin::Mensura
