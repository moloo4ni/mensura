#include "mensuraplugin.h"

#include "mensuradsp.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(MENSURA, "fy.mensura")

using namespace Qt::StringLiterals;

namespace Fooyin::Mensura {
void MensuraPlugin::initialise(const CorePluginContext& /*context*/)
{
    qCInfo(MENSURA) << "Mensura initialised";
}

std::vector<DspNode::Entry> MensuraPlugin::dspCreators() const
{
    return {{.id      = QString::fromLatin1(MensuraDsp::Id),
             .name    = u"Mensura"_s,
             .factory = [] { return std::make_unique<MensuraDsp>(); }}};
}
} // namespace Fooyin::Mensura
