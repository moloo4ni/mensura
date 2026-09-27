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
    // Every beat has the same peak; the accent differs only in pitch. Below the DSP clip knee (0.9),
    // so a click at 0 dB never gets shaped on silence.
    static constexpr double Peak        = 0.7;
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
