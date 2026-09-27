#include "beatgrid.h"

#include "mensuraparams.h"

#include <algorithm>
#include <cmath>

namespace Fooyin::Mensura {
BeatGrid::BeatGrid(double bpm, int64_t phaseNs, int beatsPerBar)
    : m_bpm{clampBpm(bpm)}
    , m_periodNs{60e9 / m_bpm}
    , m_phaseNs{phaseNs}
    , m_beatsPerBar{std::clamp(beatsPerBar, MinBeatsPerBar, MaxBeatsPerBar)}
{ }

double BeatGrid::bpm() const
{
    return m_bpm;
}

double BeatGrid::periodNs() const
{
    return m_periodNs;
}

int BeatGrid::beatsPerBar() const
{
    return m_beatsPerBar;
}

int64_t BeatGrid::beatAtOrBefore(int64_t timeNs) const
{
    const double offset = static_cast<double>(timeNs - m_phaseNs) + 1.0;
    return static_cast<int64_t>(std::floor(offset / m_periodNs));
}

double BeatGrid::beatTimeNs(int64_t beat) const
{
    return static_cast<double>(m_phaseNs) + static_cast<double>(beat) * m_periodNs;
}

int BeatGrid::indexInBar(int64_t beat) const
{
    const int64_t bar = m_beatsPerBar;
    return static_cast<int>(((beat % bar) + bar) % bar);
}
} // namespace Fooyin::Mensura
