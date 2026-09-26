#include "mensuradsp.h"

#include <core/engine/dsp/processingbufferlist.h>

using namespace Qt::StringLiterals;

namespace Fooyin::Mensura {
QString MensuraDsp::name() const
{
    return u"Mensura"_s;
}

QString MensuraDsp::id() const
{
    return QString::fromLatin1(Id);
}

void MensuraDsp::prepare(const AudioFormat& /*format*/) { }

void MensuraDsp::process(ProcessingBufferList& /*chunks*/) { }

QByteArray MensuraDsp::saveSettings() const
{
    // All settings live in the controller (SharedState), not in DSP presets.
    return {};
}

bool MensuraDsp::loadSettings(const QByteArray& /*preset*/)
{
    return true;
}
} // namespace Fooyin::Mensura
