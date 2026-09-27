#pragma once

#include <core/engine/dsp/dspplugin.h>
#include <core/plugins/coreplugin.h>
#include <core/plugins/coreplugincontext.h>
#include <core/plugins/plugin.h>
#include <gui/plugins/guiplugin.h>
#include <gui/plugins/guiplugincontext.h>

#include <QObject>
#include <QPointer>

class QTimer;

namespace Fooyin {
class PlayerController;
class SettingsManager;
} // namespace Fooyin

namespace Fooyin::Mensura {
class MensuraController;
class MensuraWindow;

class MensuraPlugin : public QObject,
                      public Plugin,
                      public CorePlugin,
                      public GuiPlugin,
                      public DspPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin/1.0" FILE "mensura.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::CorePlugin Fooyin::GuiPlugin Fooyin::DspPlugin)

public:
    void initialise(const CorePluginContext& context) override;
    void initialise(const GuiPluginContext& context) override;
    void shutdown() override;

    [[nodiscard]] std::vector<DspNode::Entry> dspCreators() const override;

private:
    void saveConfig();
    void showWindow();

    PlayerController* m_player{nullptr};
    SettingsManager* m_settings{nullptr};
    MensuraController* m_controller{nullptr};
    QTimer* m_heartbeatTimer{nullptr};
    QPointer<MensuraWindow> m_window;
};
} // namespace Fooyin::Mensura
