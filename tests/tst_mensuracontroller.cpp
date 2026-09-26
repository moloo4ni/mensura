#include "beatgrid.h"
#include "mensuracontroller.h"
#include "sharedstate.h"

#include <QSignalSpy>
#include <QTest>

#include <cmath>

using namespace Fooyin::Mensura;

namespace {
constexpr int64_t Ms = 1'000'000;

MensuraParams published(const SharedState& state)
{
    MensuraParams params;
    if(!state.tryRead(params)) {
        qFatal("SharedState snapshot unavailable");
    }
    return params;
}

// Starts playback of an untagged track at position 0, time 0
void startPlaying(MensuraController& controller)
{
    controller.handleTrackChanged(std::nullopt, 0);
    controller.handlePlayStateChanged(true, 0);
}

// Taps at the given track positions; the clock is re-based to each position first
void tapAt(MensuraController& controller, std::initializer_list<uint64_t> positionsMs)
{
    for(const uint64_t position : positionsMs) {
        const auto now = static_cast<int64_t>(position) * Ms;
        controller.handlePosition(position, now);
        controller.tap(now);
    }
}
} // namespace

class TestMensuraController : public QObject
{
    Q_OBJECT

private slots:
    void publishesOnConstruction();
    void loadConfigDoesNotEmitConfigChanged();
    void trackChangeResetsPhaseAndUsesTag();
    void manualBpmSwitchesToManual();
    void trackUpdateRecomputesTempo();
    void tapSetsManualTempoAndPhase();
    void tapInAutoWithoutTagSetsTempoKeepsMode();
    void tapInAutoWithTagOnlyShiftsPhase();
    void tapIgnoredWhileNotPlaying();
    void seekResetsTapSeries();
    void phaseOffsetAddsToTapPhase();
    void seekWhilePausedThenResume();
    void detectsMissingNode();
    void heartbeatIgnoredWhenDisabled();
    void configChangedOnlyOnRealChange();
    void volumeConvertsToGain();
    void trackChangeRestartsHeartbeatGrace();
    void seekRestartsHeartbeatGrace();
    void heartbeatGraceAfterResume();
    void invalidModeIgnored();
    void tapInManualWithTagSetsTempo();
    void tapSeriesLongerThanBar();
    void tapTempoUsesLastEightIntervals();
};

void TestMensuraController::publishesOnConstruction()
{
    SharedState state;
    const MensuraController controller{state};
    QCOMPARE(state.revision(), uint64_t{1});
    const MensuraParams params = published(state);
    QCOMPARE(params.enabled, false);
    QCOMPARE(params.bpm, 120.0);
    QCOMPARE(params.beatsPerBar, 4);
    QVERIFY(std::abs(params.gain - dbToGain(-6.0)) < 1e-12);
}

void TestMensuraController::loadConfigDoesNotEmitConfigChanged()
{
    SharedState state;
    MensuraController controller{state};
    QSignalSpy configSpy{&controller, &MensuraController::configChanged};
    QSignalSpy stateSpy{&controller, &MensuraController::stateChanged};

    MensuraConfig config;
    config.enabled   = true;
    config.manualBpm = 999.0; // must be sanitised
    controller.loadConfig(config);

    QCOMPARE(configSpy.count(), 0);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(controller.config().manualBpm, 300.0);
    QCOMPARE(published(state).enabled, true);
    QCOMPARE(published(state).bpm, 300.0);
}

void TestMensuraController::trackChangeResetsPhaseAndUsesTag()
{
    SharedState state;
    MensuraController controller{state};
    startPlaying(controller);
    tapAt(controller, {3000});
    controller.setPhaseOffsetMs(200);
    QCOMPARE(controller.phaseNs(), int64_t{3'200'000'000});

    controller.handleTrackChanged(140.0, 5000 * Ms);

    QCOMPARE(controller.phaseNs(), int64_t{0});
    QCOMPARE(controller.config().phaseOffsetMs, 0);
    QCOMPARE(controller.tempo().bpm, 140.0);
    QCOMPARE(controller.tempo().source, BpmSource::Tag);
    QCOMPARE(published(state).bpm, 140.0);
    QCOMPARE(published(state).phaseNs, int64_t{0});
    QCOMPARE(controller.positionMs(5000 * Ms), 0.0);
}

void TestMensuraController::manualBpmSwitchesToManual()
{
    SharedState state;
    MensuraController controller{state};
    controller.handleTrackChanged(140.0, 0);

    controller.setManualBpm(95.5);

    QCOMPARE(controller.config().mode, BpmMode::Manual);
    QCOMPARE(controller.tempo().bpm, 95.5);
    QCOMPARE(controller.tempo().source, BpmSource::Manual);
    QCOMPARE(published(state).bpm, 95.5);

    controller.setManualBpm(1000.0);
    QCOMPARE(controller.config().manualBpm, 300.0);
}

void TestMensuraController::trackUpdateRecomputesTempo()
{
    SharedState state;
    MensuraController controller{state};
    controller.handleTrackChanged(std::nullopt, 0);
    QCOMPARE(controller.tempo().source, BpmSource::Manual);

    QSignalSpy stateSpy{&controller, &MensuraController::stateChanged};
    controller.handleTrackUpdated(128.0);
    QCOMPARE(controller.tempo().bpm, 128.0);
    QCOMPARE(published(state).bpm, 128.0);
    QCOMPARE(stateSpy.count(), 1);

    controller.handleTrackUpdated(128.0); // unchanged: nothing happens
    QCOMPARE(stateSpy.count(), 1);
}

void TestMensuraController::tapSetsManualTempoAndPhase()
{
    SharedState state;
    MensuraController controller{state};
    controller.setMode(BpmMode::Manual);
    startPlaying(controller);

    tapAt(controller, {10000, 10600, 11200, 11800});

    QCOMPARE(controller.config().manualBpm, 100.0);
    QCOMPARE(controller.config().mode, BpmMode::Manual);
    QCOMPARE(controller.phaseNs(), int64_t{10'000'000'000}); // first tap = beat index 0

    const MensuraParams params = published(state);
    const BeatGrid grid{params.bpm, params.phaseNs, params.beatsPerBar};
    const int64_t lastBeat = grid.beatAtOrBefore(11'800'000'000);
    QCOMPARE(grid.beatTimeNs(lastBeat), 11'800'000'000.0); // grid passes through the last tap
    QCOMPARE(grid.indexInBar(lastBeat), 3);
}

void TestMensuraController::tapInAutoWithoutTagSetsTempoKeepsMode()
{
    SharedState state;
    MensuraController controller{state};
    startPlaying(controller);

    tapAt(controller, {2000, 2500});

    QCOMPARE(controller.config().mode, BpmMode::Auto);
    QCOMPARE(controller.config().manualBpm, 120.0);
    tapAt(controller, {3250});
    QCOMPARE(controller.config().manualBpm, 96.0); // mean of 500 and 750 ms
    QCOMPARE(controller.config().mode, BpmMode::Auto);
}

void TestMensuraController::tapInAutoWithTagOnlyShiftsPhase()
{
    SharedState state;
    MensuraController controller{state};
    controller.handleTrackChanged(150.0, 0);
    controller.handlePlayStateChanged(true, 0);

    tapAt(controller, {10000, 10500});

    QCOMPARE(controller.tempo().bpm, 150.0);
    QCOMPARE(controller.config().manualBpm, 120.0);
    // Last tap at 10500 is beat index 1; tag period is 400 ms
    QCOMPARE(controller.phaseNs(), int64_t{10'100'000'000});
}

void TestMensuraController::tapIgnoredWhileNotPlaying()
{
    SharedState state;
    MensuraController controller{state};
    controller.handleTrackChanged(std::nullopt, 0);
    const uint64_t revision = state.revision();

    tapAt(controller, {1000, 1500});

    QCOMPARE(controller.phaseNs(), int64_t{0});
    QCOMPARE(controller.config().manualBpm, 120.0);
    QCOMPARE(state.revision(), revision);
}

void TestMensuraController::seekResetsTapSeries()
{
    SharedState state;
    MensuraController controller{state};
    startPlaying(controller);

    tapAt(controller, {10000});
    controller.handleSeek(10400, 10400 * Ms);
    controller.tap(10400 * Ms);

    QCOMPARE(controller.config().manualBpm, 120.0); // no 150 BPM from a cross-seek interval
    QCOMPARE(controller.phaseNs(), int64_t{10'400'000'000});
}

void TestMensuraController::phaseOffsetAddsToTapPhase()
{
    SharedState state;
    MensuraController controller{state};
    startPlaying(controller);
    tapAt(controller, {4000});

    controller.setPhaseOffsetMs(250);
    QCOMPARE(controller.phaseNs(), int64_t{4'250'000'000});
    QCOMPARE(published(state).phaseNs, int64_t{4'250'000'000});

    controller.setPhaseOffsetMs(-5000);
    QCOMPARE(controller.config().phaseOffsetMs, -1000);

    controller.resetPhase();
    QCOMPARE(controller.config().phaseOffsetMs, 0);
    QCOMPARE(controller.phaseNs(), int64_t{0});
    QCOMPARE(published(state).phaseNs, int64_t{0});
}

void TestMensuraController::seekWhilePausedThenResume()
{
    SharedState state;
    MensuraController controller{state};
    controller.handleTrackChanged(std::nullopt, 0);
    controller.handlePosition(1000, 0);
    controller.handlePlayStateChanged(true, 0);
    controller.handlePlayStateChanged(false, 300 * Ms);
    controller.handleSeek(5000, 1000 * Ms);
    controller.handlePlayStateChanged(true, 2000 * Ms);

    controller.tap(2100 * Ms);

    QCOMPARE(controller.phaseNs(), int64_t{5'100'000'000});
}

void TestMensuraController::detectsMissingNode()
{
    SharedState state;
    MensuraController controller{state};
    controller.setEnabled(true);
    QSignalSpy spy{&controller, &MensuraController::nodeMissingChanged};

    constexpr int64_t Start = 10'000 * Ms;
    controller.handlePlayStateChanged(true, Start);

    controller.checkHeartbeat(Start + 500 * Ms);
    QCOMPARE(controller.nodeMissing(), false);
    QCOMPARE(spy.count(), 0);

    controller.checkHeartbeat(Start + 1500 * Ms);
    QCOMPARE(controller.nodeMissing(), true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);

    controller.checkHeartbeat(Start + 1600 * Ms); // still missing: no repeated signal
    QCOMPARE(spy.count(), 1);

    state.heartbeat(Start + 1700 * Ms);
    controller.checkHeartbeat(Start + 1800 * Ms);
    QCOMPARE(controller.nodeMissing(), false);
    QCOMPARE(spy.count(), 2);

    controller.checkHeartbeat(Start + 5000 * Ms);
    QCOMPARE(controller.nodeMissing(), true);
    controller.handlePlayStateChanged(false, Start + 5100 * Ms);
    controller.checkHeartbeat(Start + 5200 * Ms); // stopped: warning cleared
    QCOMPARE(controller.nodeMissing(), false);
}

void TestMensuraController::heartbeatIgnoredWhenDisabled()
{
    SharedState state;
    MensuraController controller{state};
    controller.handlePlayStateChanged(true, 0);
    controller.checkHeartbeat(10'000 * Ms);
    QCOMPARE(controller.nodeMissing(), false);
}

void TestMensuraController::configChangedOnlyOnRealChange()
{
    SharedState state;
    MensuraController controller{state};
    QSignalSpy spy{&controller, &MensuraController::configChanged};

    controller.setBeatsPerBar(4); // default: no change
    controller.setSound(static_cast<ClickSound>(9));
    QCOMPARE(spy.count(), 0);

    controller.setBeatsPerBar(40);
    QCOMPARE(controller.config().beatsPerBar, 16);
    controller.setSound(ClickSound::Beep);
    controller.setEnabled(true);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(published(state).beatsPerBar, 16);
    QCOMPARE(published(state).sound, ClickSound::Beep);
    QCOMPARE(published(state).enabled, true);
}

void TestMensuraController::volumeConvertsToGain()
{
    SharedState state;
    MensuraController controller{state};
    controller.setVolumeDb(-20.0);
    QVERIFY(std::abs(published(state).gain - 0.1) < 1e-12);
    controller.setVolumeDb(10.0);
    QCOMPARE(controller.config().volumeDb, 0.0);
}

void TestMensuraController::trackChangeRestartsHeartbeatGrace()
{
    SharedState state;
    MensuraController controller{state};
    controller.setEnabled(true);
    controller.handlePlayStateChanged(true, 0);
    state.heartbeat(500 * Ms); // healthy, then the pipeline stalls

    constexpr int64_t T = 10'000 * Ms;
    controller.handleTrackChanged(std::nullopt, T);
    controller.checkHeartbeat(T + 900 * Ms);
    QCOMPARE(controller.nodeMissing(), false);
    controller.checkHeartbeat(T + 1100 * Ms);
    QCOMPARE(controller.nodeMissing(), true);
}

void TestMensuraController::seekRestartsHeartbeatGrace()
{
    SharedState state;
    MensuraController controller{state};
    controller.setEnabled(true);
    controller.handlePlayStateChanged(true, 0);
    state.heartbeat(500 * Ms);

    constexpr int64_t T = 10'000 * Ms;
    controller.handleSeek(60'000, T);
    controller.checkHeartbeat(T + 900 * Ms);
    QCOMPARE(controller.nodeMissing(), false);
    controller.checkHeartbeat(T + 1100 * Ms);
    QCOMPARE(controller.nodeMissing(), true);
}

void TestMensuraController::heartbeatGraceAfterResume()
{
    SharedState state;
    MensuraController controller{state};
    controller.setEnabled(true);
    controller.handlePlayStateChanged(true, 0);
    state.heartbeat(100 * Ms);
    controller.handlePlayStateChanged(false, 200 * Ms);

    constexpr int64_t T = 60'000 * Ms;
    controller.handlePlayStateChanged(true, T);
    controller.checkHeartbeat(T + 500 * Ms);
    QCOMPARE(controller.nodeMissing(), false);
}

void TestMensuraController::invalidModeIgnored()
{
    SharedState state;
    MensuraController controller{state};
    QSignalSpy spy{&controller, &MensuraController::configChanged};
    const uint64_t revision = state.revision();

    controller.setMode(static_cast<BpmMode>(7));

    QCOMPARE(controller.config().mode, BpmMode::Auto);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(state.revision(), revision);
}

void TestMensuraController::tapInManualWithTagSetsTempo()
{
    SharedState state;
    MensuraController controller{state};
    controller.handleTrackChanged(150.0, 0);
    controller.setMode(BpmMode::Manual);
    controller.handlePlayStateChanged(true, 0);

    tapAt(controller, {10000, 10600});

    QCOMPARE(controller.config().mode, BpmMode::Manual);
    QCOMPARE(controller.config().manualBpm, 100.0);
    QCOMPARE(controller.tempo().bpm, 100.0);
    QCOMPARE(controller.phaseNs(), int64_t{10'000'000'000});
    QCOMPARE(published(state).bpm, 100.0);
}

void TestMensuraController::tapSeriesLongerThanBar()
{
    SharedState state;
    MensuraController controller{state};
    controller.setMode(BpmMode::Manual);
    startPlaying(controller);

    tapAt(controller, {10000, 10500, 11000, 11500, 12000});

    const MensuraParams params = published(state);
    QCOMPARE(params.bpm, 120.0);
    const BeatGrid grid{params.bpm, params.phaseNs, params.beatsPerBar};
    const int64_t lastBeat = grid.beatAtOrBefore(12'000'000'000);
    QCOMPARE(grid.beatTimeNs(lastBeat), 12'000'000'000.0);
    QCOMPARE(grid.indexInBar(lastBeat), 0); // (5 - 1) mod 4
}

void TestMensuraController::tapTempoUsesLastEightIntervals()
{
    SharedState state;
    MensuraController controller{state};
    controller.setMode(BpmMode::Manual);
    startPlaying(controller);

    // One 1000 ms interval followed by eight 500 ms intervals
    tapAt(controller, {10000, 11000, 11500, 12000, 12500, 13000, 13500, 14000, 14500, 15000});

    QCOMPARE(controller.config().manualBpm, 120.0);
    // Last tap is beat index 9 of the series
    QCOMPARE(controller.phaseNs(), int64_t{10'500'000'000});
    const MensuraParams params = published(state);
    const BeatGrid grid{params.bpm, params.phaseNs, params.beatsPerBar};
    const int64_t lastBeat = grid.beatAtOrBefore(15'000'000'000);
    QCOMPARE(grid.beatTimeNs(lastBeat), 15'000'000'000.0);
    QCOMPARE(grid.indexInBar(lastBeat), 1); // 9 mod 4
}

QTEST_GUILESS_MAIN(TestMensuraController)
#include "tst_mensuracontroller.moc"
