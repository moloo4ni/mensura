#include "beatindicator.h"
#include "mensuracontroller.h"
#include "mensurapanel.h"
#include "sharedstate.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTest>
#include <QTimer>

#include <cstdlib>

using namespace Fooyin::Mensura;
using namespace Qt::StringLiterals;

namespace {
constexpr int64_t Ms = 1'000'000;

template <typename T>
T* child(const QWidget& parent, const QString& name)
{
    auto* widget = parent.findChild<T*>(name);
    Q_ASSERT(widget);
    return widget;
}
} // namespace

class TestMensuraPanel : public QObject
{
    Q_OBJECT

private slots:
    void showsControllerState();
    void editingBpmSwitchesToManual();
    void controlsDriveController();
    void tapEnabledOnlyWhilePlaying();
    void tapButtonTaps();
    void tapKeyWorksFromSpinBoxes();
    void timerRunsOnlyWhileVisibleAndPlaying();
    void timerStopsWhenMinimised();
    void refreshKeepsTypedInput();
    void warningFollowsNodeMissing();
    void indicatorShowsCurrentBeat();
    void indicatorClampsBeatCount();
};

void TestMensuraPanel::showsControllerState()
{
    SharedState state;
    MensuraController controller{state};
    controller.handleTrackChanged(128.0, 0); // before loadConfig: a track change zeroes the offset
    MensuraConfig config;
    config.enabled       = true;
    config.beatsPerBar   = 3;
    config.sound         = ClickSound::Wood;
    config.volumeDb      = -12.0;
    config.phaseOffsetMs = 40;
    controller.loadConfig(config);

    const MensuraPanel panel{&controller};

    QVERIFY(child<QCheckBox>(panel, u"enabled"_s)->isChecked());
    QCOMPARE(child<QDoubleSpinBox>(panel, u"bpm"_s)->value(), 128.0);
    QCOMPARE(child<QLabel>(panel, u"source"_s)->text(), u"tag"_s);
    QCOMPARE(child<QComboBox>(panel, u"mode"_s)->currentIndex(), 0);
    QCOMPARE(child<QSpinBox>(panel, u"beats"_s)->value(), 3);
    QCOMPARE(child<QComboBox>(panel, u"sound"_s)->currentIndex(), 1);
    QCOMPARE(child<QSlider>(panel, u"volume"_s)->value(), -12);
    QCOMPARE(child<QLabel>(panel, u"volumeLabel"_s)->text(), u"-12 dB"_s);
    QCOMPARE(child<QSpinBox>(panel, u"offset"_s)->value(), 40);
    QCOMPARE(child<BeatIndicator>(panel, u"indicator"_s)->beatCount(), 3);

    controller.handleTrackChanged(std::nullopt, 0); // stateChanged -> refresh
    QCOMPARE(child<QDoubleSpinBox>(panel, u"bpm"_s)->value(), 120.0);
    QCOMPARE(controller.config().mode, BpmMode::Auto); // refresh must not feed back (setManualBpm)
    QCOMPARE(child<QLabel>(panel, u"source"_s)->text(), u"manual"_s);
}

void TestMensuraPanel::editingBpmSwitchesToManual()
{
    SharedState state;
    MensuraController controller{state};
    controller.handleTrackChanged(140.0, 0);
    const MensuraPanel panel{&controller};

    child<QDoubleSpinBox>(panel, u"bpm"_s)->setValue(100.0);

    QCOMPARE(controller.config().mode, BpmMode::Manual);
    QCOMPARE(controller.config().manualBpm, 100.0);
    QCOMPARE(child<QComboBox>(panel, u"mode"_s)->currentIndex(), 1);
    QCOMPARE(child<QLabel>(panel, u"source"_s)->text(), u"manual"_s);
}

void TestMensuraPanel::controlsDriveController()
{
    SharedState state;
    MensuraController controller{state};
    const MensuraPanel panel{&controller};

    child<QCheckBox>(panel, u"enabled"_s)->setChecked(true);
    QCOMPARE(controller.config().enabled, true);

    child<QComboBox>(panel, u"mode"_s)->setCurrentIndex(1);
    QCOMPARE(controller.config().mode, BpmMode::Manual);

    child<QSpinBox>(panel, u"beats"_s)->setValue(7);
    QCOMPARE(controller.config().beatsPerBar, 7);
    QCOMPARE(child<BeatIndicator>(panel, u"indicator"_s)->beatCount(), 7);

    child<QComboBox>(panel, u"sound"_s)->setCurrentIndex(2);
    QCOMPARE(controller.config().sound, ClickSound::Beep);

    child<QSlider>(panel, u"volume"_s)->setValue(-20);
    QCOMPARE(controller.config().volumeDb, -20.0);
    QCOMPARE(child<QLabel>(panel, u"volumeLabel"_s)->text(), u"-20 dB"_s);

    child<QSpinBox>(panel, u"offset"_s)->setValue(250);
    QCOMPARE(controller.config().phaseOffsetMs, 250);

    child<QPushButton>(panel, u"resetPhase"_s)->click();
    QCOMPARE(controller.config().phaseOffsetMs, 0);
    QCOMPARE(child<QSpinBox>(panel, u"offset"_s)->value(), 0);
}

void TestMensuraPanel::tapEnabledOnlyWhilePlaying()
{
    SharedState state;
    MensuraController controller{state};
    const MensuraPanel panel{&controller};
    auto* tap = child<QPushButton>(panel, u"tap"_s);

    QVERIFY(!tap->isEnabled());
    controller.handlePlayStateChanged(true, 0);
    QVERIFY(tap->isEnabled());
    controller.handlePlayStateChanged(false, 100 * Ms);
    QVERIFY(!tap->isEnabled());
}

void TestMensuraPanel::tapButtonTaps()
{
    SharedState state;
    MensuraController controller{state};
    const MensuraPanel panel{&controller};
    controller.handleTrackChanged(std::nullopt, 0);
    const int64_t now = SharedState::nowNs();
    controller.handlePlayStateChanged(true, now);
    controller.handlePosition(5000, now);
    QCOMPARE(controller.phaseNs(), 0);
    QSignalSpy spy{&controller, &MensuraController::stateChanged};

    emit child<QPushButton>(panel, u"tap"_s)->pressed();

    // One tap at the current position (≈ 5000 ms): the phase is that position
    QCOMPARE(spy.count(), 1);
    QVERIFY(controller.phaseNs() >= 5000 * Ms);
    QVERIFY(controller.phaseNs() < 6000 * Ms);
}

void TestMensuraPanel::tapKeyWorksFromSpinBoxes()
{
    SharedState state;
    MensuraController controller{state};
    MensuraPanel panel{&controller};
    controller.handleTrackChanged(std::nullopt, 0);
    controller.handlePlayStateChanged(true, SharedState::nowNs());
    panel.show();
    panel.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&panel));

    for(const QString& name : {u"bpm"_s, u"offset"_s}) {
        auto* box = child<QAbstractSpinBox>(panel, name);
        box->setFocus();
        QVERIFY(box->hasFocus());
        const QString textBefore = box->text();
        QSignalSpy spy{&controller, &MensuraController::stateChanged};

        QTest::keyClick(box, Qt::Key_T);

        QCOMPARE(spy.count(), 1); // exactly one tap
        QCOMPARE(box->text(), textBefore);
    }
}

void TestMensuraPanel::timerRunsOnlyWhileVisibleAndPlaying()
{
    SharedState state;
    MensuraController controller{state};
    MensuraPanel panel{&controller};
    auto* timer = panel.findChild<QTimer*>();
    QVERIFY(timer);

    panel.show();
    QVERIFY(!timer->isActive()); // visible, paused

    controller.handlePlayStateChanged(true, 0);
    QVERIFY(timer->isActive()); // visible, playing

    panel.hide();
    QVERIFY(!timer->isActive()); // hidden, playing

    panel.show();
    QVERIFY(timer->isActive());
    controller.handlePlayStateChanged(false, 100 * Ms);
    QVERIFY(!timer->isActive()); // paused again
    QCOMPARE(child<BeatIndicator>(panel, u"indicator"_s)->currentBeat(), -1);
}

void TestMensuraPanel::warningFollowsNodeMissing()
{
    SharedState state;
    MensuraController controller{state};
    const MensuraPanel panel{&controller};
    auto* warning = child<QLabel>(panel, u"warning"_s);
    QVERIFY(warning->isHidden());

    controller.setEnabled(true);
    controller.handlePlayStateChanged(true, 10'000 * Ms);
    controller.checkHeartbeat(12'000 * Ms);
    QVERIFY(!warning->isHidden());

    state.heartbeat(12'100 * Ms);
    controller.checkHeartbeat(12'200 * Ms);
    QVERIFY(warning->isHidden());
}

void TestMensuraPanel::indicatorShowsCurrentBeat()
{
    SharedState state;
    MensuraController controller{state};
    MensuraPanel panel{&controller};
    auto* indicator = child<BeatIndicator>(panel, u"indicator"_s);

    controller.handleTrackChanged(std::nullopt, 0);
    controller.handlePosition(1600, 0);
    panel.updateIndicator(0);
    QCOMPARE(indicator->currentBeat(), -1); // not playing

    controller.handlePlayStateChanged(true, 0);
    panel.updateIndicator(0);
    QCOMPARE(indicator->currentBeat(), 3); // 120 BPM: beat 3 starts at 1500 ms

    panel.updateIndicator(500 * Ms); // 2100 ms -> beat 4 -> index 0
    QCOMPARE(indicator->currentBeat(), 0);
}

void TestMensuraPanel::indicatorClampsBeatCount()
{
    BeatIndicator indicator;
    indicator.setBeatCount(4);
    indicator.setCurrentBeat(3);
    indicator.setBeatCount(40);
    QCOMPARE(indicator.beatCount(), 16);
    QCOMPARE(indicator.currentBeat(), 3);

    indicator.setBeatCount(2); // current beat out of range -> none
    QCOMPARE(indicator.currentBeat(), -1);
    indicator.setBeatCount(0);
    QCOMPARE(indicator.beatCount(), 1);
}

void TestMensuraPanel::timerStopsWhenMinimised()
{
    SharedState state;
    MensuraController controller{state};
    MensuraPanel panel{&controller};
    auto* timer = panel.findChild<QTimer*>();
    controller.handlePlayStateChanged(true, 0);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    QVERIFY(timer->isActive());

    panel.showMinimized(); // spontaneous hide: isVisible() stays true
    QTRY_VERIFY(!timer->isActive());

    panel.showNormal();
    QTRY_VERIFY(timer->isActive());
}

void TestMensuraPanel::refreshKeepsTypedInput()
{
    SharedState state;
    MensuraController controller{state};
    const MensuraPanel panel{&controller};

    for(const QString& name : {u"bpm"_s, u"offset"_s}) {
        auto* edit = child<QAbstractSpinBox>(panel, name)->findChild<QLineEdit*>();
        QVERIFY(edit);
        edit->selectAll();
        QTest::keyClicks(edit, u"9"_s); // not committed: keyboard tracking is off
        const QString typed = edit->text();

        controller.setEnabled(!controller.config().enabled); // stateChanged -> refresh, values unchanged

        QCOMPARE(edit->text(), typed);
    }
}

QTEST_MAIN(TestMensuraPanel)
#include "tst_mensurapanel.moc"
