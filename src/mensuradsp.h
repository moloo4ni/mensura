#pragma once

#include <core/engine/dsp/dspnode.h>

namespace Fooyin::Mensura {
class MensuraDsp : public DspNode
{
public:
    static constexpr auto Id = "fooyin.dsp.mensura";

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString id() const override;

    void prepare(const AudioFormat& format) override;
    void process(ProcessingBufferList& chunks) override;

    [[nodiscard]] QByteArray saveSettings() const override;
    bool loadSettings(const QByteArray& preset) override;
};
} // namespace Fooyin::Mensura
