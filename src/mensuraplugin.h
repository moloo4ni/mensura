#pragma once

#include <core/engine/dsp/dspplugin.h>
#include <core/plugins/coreplugin.h>
#include <core/plugins/coreplugincontext.h>
#include <core/plugins/plugin.h>

#include <QObject>

namespace Fooyin::Mensura {
class MensuraPlugin : public QObject,
                      public Plugin,
                      public CorePlugin,
                      public DspPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin/1.0" FILE "mensura.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::CorePlugin Fooyin::DspPlugin)

public:
    void initialise(const CorePluginContext& context) override;

    [[nodiscard]] std::vector<DspNode::Entry> dspCreators() const override;
};
} // namespace Fooyin::Mensura
