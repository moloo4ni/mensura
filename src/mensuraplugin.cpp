#include "mensuraplugin.h"

#include "mensuracontroller.h"
#include "mensuradsp.h"
#include "mensurawindow.h"
#include "sharedstate.h"
#include "temporesolver.h"

#include <core/player/playercontroller.h>
#include <core/track.h>
#include <gui/guiconstants.h>
#include <utils/actions/actioncontainer.h>
#include <utils/actions/actionmanager.h>
#include <utils/actions/command.h>
#include <utils/settings/settingsmanager.h>

#include <QAction>
#include <QLoggingCategory>
#include <QTimer>

Q_LOGGING_CATEGORY(MENSURA, "fy.mensura")

using namespace Qt::StringLiterals;

namespace Fooyin::Mensura {
namespace {
constexpr auto ConfigKey        = "Mensura/Config";
constexpr auto WindowStateKey   = "Mensura/WindowState";
constexpr auto ShowWindowId     = "Mensura.ShowWindow";
constexpr int HeartbeatCheckMs  = 500;
const QSize DefaultWindowSize{420, 340};
} // namespace

void MensuraPlugin::initialise(const CorePluginContext& context)
{
    m_player     = context.playerController;
    m_settings   = context.settingsManager;
    m_controller = new MensuraController(SharedState::instance(), this);

    m_controller->loadConfig(MensuraConfig::fromMap(m_settings->fileValue(ConfigKey).toMap()));
    connect(m_controller, &MensuraController::configChanged, this, &MensuraPlugin::saveConfig);

    connect(m_player, &PlayerController::currentTrackChanged, this, [this](const Track& track) {
        m_controller->handleTrackChanged(trackBpm(track), SharedState::nowNs());
    });
    connect(m_player, &PlayerController::currentTrackUpdated, this,
            [this](const Track& track) { m_controller->handleTrackUpdated(trackBpm(track)); });
    connect(m_player, &PlayerController::playStateChanged, this, [this](Player::PlayState state) {
        m_controller->handlePlayStateChanged(state == Player::PlayState::Playing, SharedState::nowNs());
    });
    connect(m_player, &PlayerController::positionChanged, this,
            [this](uint64_t ms) { m_controller->handlePosition(ms, SharedState::nowNs()); });
    connect(m_player, &PlayerController::positionMoved, this,
            [this](uint64_t ms) { m_controller->handleSeek(ms, SharedState::nowNs()); });

    // Plugins may be initialised while a track is already loaded (session restore)
    const int64_t now = SharedState::nowNs();
    m_controller->handleTrackChanged(trackBpm(m_player->currentTrack()), now);
    m_controller->handlePosition(m_player->currentPosition(), now);
    m_controller->handlePlayStateChanged(m_player->playState() == Player::PlayState::Playing, now);

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(HeartbeatCheckMs);
    connect(m_heartbeatTimer, &QTimer::timeout, this,
            [this] { m_controller->checkHeartbeat(SharedState::nowNs()); });
    m_heartbeatTimer->start();

    qCInfo(MENSURA) << "Mensura initialised";
}

void MensuraPlugin::initialise(const GuiPluginContext& context)
{
    auto* action = new QAction(tr("&Mensura"), this);
    connect(action, &QAction::triggered, this, &MensuraPlugin::showWindow);

    Command* command = context.actionManager->registerAction(action, ShowWindowId);
    command->setCategories({tr("View")});

    if(auto* viewMenu = context.actionManager->actionContainer(Constants::Menus::View)) {
        viewMenu->addAction(command);
    }
    else {
        qCWarning(MENSURA) << "View menu not found; the Mensura window is only reachable via shortcuts";
    }
}

void MensuraPlugin::shutdown()
{
    if(m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }
    if(m_controller) {
        saveConfig();
    }
    delete m_window; // QPointer: no-op if fooyin already destroyed it
}

std::vector<DspNode::Entry> MensuraPlugin::dspCreators() const
{
    return {{.id      = QString::fromLatin1(MensuraDsp::Id),
             .name    = u"Mensura"_s,
             .factory = [] { return std::make_unique<MensuraDsp>(); }}};
}

void MensuraPlugin::saveConfig()
{
    m_settings->fileSet(ConfigKey, m_controller->config().toMap());
}

void MensuraPlugin::showWindow()
{
    if(m_window) {
        // Already open: only bring it to the front. Calling showStandaloneWindow again on the
        // same instance would re-apply the geometry saved at the last close.
        m_window->setWindowState(m_window->windowState() & ~Qt::WindowMinimized);
        m_window->show();
        m_window->raise();
        m_window->activateWindow();
        return;
    }
    // fooyin sets WA_DeleteOnClose and saves the geometry in closeEvent, so a closed window is
    // gone (the QPointer is null) and a new one restores the saved geometry here
    m_window = new MensuraWindow(m_controller);
    m_window->showStandaloneWindow(tr("Mensura"), QString::fromLatin1(WindowStateKey), DefaultWindowSize);
}
} // namespace Fooyin::Mensura
