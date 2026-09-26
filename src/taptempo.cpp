#include "taptempo.h"

#include "mensuraparams.h"

namespace Fooyin::Mensura {
void TapTempo::tap(double positionMs)
{
    if(!m_taps.empty()) {
        const double gap = positionMs - m_taps.back();
        if(gap > ResetGapMs || gap <= 0.0) {
            reset();
        }
    }

    m_taps.push_back(positionMs);
    ++m_count;
    if(m_taps.size() > static_cast<size_t>(MaxIntervals) + 1) {
        m_taps.erase(m_taps.begin());
    }
}

void TapTempo::reset()
{
    m_taps.clear();
    m_count = 0;
}

std::optional<double> TapTempo::bpm() const
{
    if(m_taps.size() < 2) {
        return {};
    }
    const double interval = (m_taps.back() - m_taps.front()) / static_cast<double>(m_taps.size() - 1);
    return clampBpm(60'000.0 / interval);
}

int TapTempo::tapCount() const
{
    return m_count;
}
} // namespace Fooyin::Mensura
