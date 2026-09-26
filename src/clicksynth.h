#pragma once

#include "mensuraparams.h"

#include <array>
#include <span>
#include <vector>

namespace Fooyin::Mensura {
//! Pre-synthesised mono click buffers for every sound, normal and accented.
class ClickSynth
{
public:
    static constexpr double NormalPeak  = 0.5;
    static constexpr double AccentPeak  = 1.0; // +6 dB over a normal beat
    static constexpr double AccentPitch = 1.5;

    explicit ClickSynth(int sampleRate);

    [[nodiscard]] int sampleRate() const;
    //! Never allocates; out-of-range sounds fall back to Click.
    [[nodiscard]] std::span<const double> click(ClickSound sound, bool accent) const;

private:
    int m_sampleRate;
    std::array<std::vector<double>, static_cast<size_t>(ClickSoundCount) * 2> m_clicks;
};
} // namespace Fooyin::Mensura
