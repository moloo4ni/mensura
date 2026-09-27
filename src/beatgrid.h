#pragma once

#include <cstdint>

namespace Fooyin::Mensura {
//! Beat n sounds at phaseNs + n * 60e9 / bpm (track time, ns, n may be negative).
class BeatGrid
{
public:
    BeatGrid(double bpm, int64_t phaseNs, int beatsPerBar);

    [[nodiscard]] double bpm() const;
    [[nodiscard]] double periodNs() const;
    [[nodiscard]] int beatsPerBar() const;

    //! Last beat at or before timeNs; a time 1 ns before a beat counts as that beat.
    [[nodiscard]] int64_t beatAtOrBefore(int64_t timeNs) const;
    [[nodiscard]] double beatTimeNs(int64_t beat) const;
    //! Position within the bar, 0 = downbeat. Floor semantics for negative beats.
    [[nodiscard]] int indexInBar(int64_t beat) const;

private:
    double m_bpm;
    double m_periodNs;
    int64_t m_phaseNs;
    int m_beatsPerBar;
};
} // namespace Fooyin::Mensura
