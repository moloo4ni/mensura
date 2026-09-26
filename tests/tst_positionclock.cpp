#include "positionclock.h"

#include <QTest>

using namespace Fooyin::Mensura;

namespace {
constexpr int64_t Ms = 1'000'000;
}

class TestPositionClock : public QObject
{
    Q_OBJECT

private slots:
    void interpolatesWhilePlaying();
    void frozenWhileStopped();
    void pauseFreezesInterpolatedPosition();
    void seekWhilePausedThenResume();
    void positionUpdateRebases();
    void neverRunsBackwards();
};

void TestPositionClock::interpolatesWhilePlaying()
{
    PositionClock clock;
    clock.setPosition(1000, 0);
    clock.setPlaying(true, 0);
    QVERIFY(clock.isPlaying());
    QCOMPARE(clock.positionMs(250 * Ms), 1250.0);
}

void TestPositionClock::frozenWhileStopped()
{
    PositionClock clock;
    clock.setPosition(1000, 0);
    QVERIFY(!clock.isPlaying());
    QCOMPARE(clock.positionMs(5000 * Ms), 1000.0);
}

void TestPositionClock::pauseFreezesInterpolatedPosition()
{
    PositionClock clock;
    clock.setPosition(1000, 0);
    clock.setPlaying(true, 0);
    clock.setPlaying(false, 300 * Ms);
    QCOMPARE(clock.positionMs(300 * Ms), 1300.0);
    QCOMPARE(clock.positionMs(9000 * Ms), 1300.0);
}

void TestPositionClock::seekWhilePausedThenResume()
{
    PositionClock clock;
    clock.setPosition(1000, 0);
    clock.setPlaying(true, 0);
    clock.setPlaying(false, 300 * Ms);

    clock.setPosition(5000, 1000 * Ms); // seek while paused
    QCOMPARE(clock.positionMs(1500 * Ms), 5000.0);

    clock.setPlaying(true, 2000 * Ms);
    QCOMPARE(clock.positionMs(2100 * Ms), 5100.0);
}

void TestPositionClock::positionUpdateRebases()
{
    PositionClock clock;
    clock.setPlaying(true, 0);
    clock.setPosition(2000, 100 * Ms);
    QCOMPARE(clock.positionMs(150 * Ms), 2050.0);
}

void TestPositionClock::neverRunsBackwards()
{
    PositionClock clock;
    clock.setPosition(2000, 100 * Ms);
    clock.setPlaying(true, 100 * Ms);
    QCOMPARE(clock.positionMs(50 * Ms), 2000.0);
}

QTEST_APPLESS_MAIN(TestPositionClock)
#include "tst_positionclock.moc"
