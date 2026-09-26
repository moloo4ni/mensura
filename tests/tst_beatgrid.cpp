#include "beatgrid.h"

#include <QTest>

#include <cmath>
#include <limits>

using namespace Fooyin::Mensura;

class TestBeatGrid : public QObject
{
    Q_OBJECT

private slots:
    void periodMatchesBpm();
    void beatOnBoundaryBelongsToIt();
    void negativeTimeAndPhase();
    void indexInBarWrapsNegativeBeats();
    void singleBeatBarIsAlwaysZero();
    void fractionalBpmRoundTrips();
    void noDriftOverOneHour();
    void clampsParameters();
};

void TestBeatGrid::periodMatchesBpm()
{
    QCOMPARE(BeatGrid(120.0, 0, 4).periodNs(), 500'000'000.0);
    QCOMPARE(BeatGrid(60.0, 0, 4).periodNs(), 1'000'000'000.0);
}

void TestBeatGrid::beatOnBoundaryBelongsToIt()
{
    const BeatGrid grid{120.0, 0, 4};
    QCOMPARE(grid.beatAtOrBefore(0), int64_t{0});
    QCOMPARE(grid.beatAtOrBefore(499'999'998), int64_t{0});
    QCOMPARE(grid.beatAtOrBefore(499'999'999), int64_t{1}); // 1 ns tolerance
    QCOMPARE(grid.beatAtOrBefore(500'000'000), int64_t{1});
    QCOMPARE(grid.beatTimeNs(1), 500'000'000.0);
}

void TestBeatGrid::negativeTimeAndPhase()
{
    const BeatGrid late{120.0, 100'000'000, 4};
    QCOMPARE(late.beatAtOrBefore(0), int64_t{-1});
    QCOMPARE(late.beatTimeNs(-1), -400'000'000.0);

    const BeatGrid early{120.0, -250'000'000, 4};
    QCOMPARE(early.beatAtOrBefore(0), int64_t{0});
    QCOMPARE(early.beatTimeNs(0), -250'000'000.0);
    QCOMPARE(early.beatAtOrBefore(-300'000'000), int64_t{-1});
}

void TestBeatGrid::indexInBarWrapsNegativeBeats()
{
    const BeatGrid grid{120.0, 0, 4};
    QCOMPARE(grid.indexInBar(0), 0);
    QCOMPARE(grid.indexInBar(3), 3);
    QCOMPARE(grid.indexInBar(4), 0);
    QCOMPARE(grid.indexInBar(-1), 3);
    QCOMPARE(grid.indexInBar(-4), 0);
    QCOMPARE(grid.indexInBar(-5), 3);
}

void TestBeatGrid::singleBeatBarIsAlwaysZero()
{
    const BeatGrid grid{120.0, 0, 1};
    for(int64_t beat = -3; beat <= 3; ++beat) {
        QCOMPARE(grid.indexInBar(beat), 0);
    }
}

void TestBeatGrid::fractionalBpmRoundTrips()
{
    const BeatGrid grid{127.9, 12'345, 7};
    for(int64_t beat = -100; beat < 100'000; beat += 997) {
        const auto time = std::llround(grid.beatTimeNs(beat));
        QCOMPARE(grid.beatAtOrBefore(time), beat);
        QCOMPARE(grid.beatAtOrBefore(time - 2), beat - 1);
    }
}

void TestBeatGrid::noDriftOverOneHour()
{
    QCOMPARE(BeatGrid(120.0, 0, 4).beatTimeNs(7200), 3'600'000'000'000.0);

    const BeatGrid grid{127.9, 0, 4};
    const int64_t beat = 7674; // ~1 hour
    const double exact = static_cast<double>(beat) * 60e9 / 127.9;
    QVERIFY(std::abs(grid.beatTimeNs(beat) - exact) < 1.0);
}

void TestBeatGrid::clampsParameters()
{
    const BeatGrid low{5.0, 0, 0};
    QCOMPARE(low.bpm(), 20.0);
    QCOMPARE(low.beatsPerBar(), 1);

    const BeatGrid high{1000.0, 0, 99};
    QCOMPARE(high.bpm(), 300.0);
    QCOMPARE(high.beatsPerBar(), 16);

    QCOMPARE(BeatGrid(std::numeric_limits<double>::quiet_NaN(), 0, 4).bpm(), 120.0);
}

QTEST_APPLESS_MAIN(TestBeatGrid)
#include "tst_beatgrid.moc"
