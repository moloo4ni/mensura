#pragma once

#include <optional>
#include <vector>

namespace Fooyin::Mensura {
//! Tempo from a series of taps, timestamped with the track position.
class TapTempo
{
public:
    static constexpr double ResetGapMs = 2000.0;
    static constexpr int MaxIntervals  = 8;

    void tap(double positionMs);
    void reset();

    //! Needs at least two taps; mean of the last MaxIntervals intervals, clamped.
    [[nodiscard]] std::optional<double> bpm() const;
    //! All taps of the current series, including ones no longer used for the mean.
    [[nodiscard]] int tapCount() const;

private:
    std::vector<double> m_taps; // the last MaxIntervals + 1 taps
    int m_count{0};
};
} // namespace Fooyin::Mensura
