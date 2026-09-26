#include "taptempo.h"

#include <QTest>

using namespace Fooyin::Mensura;

class TestTapTempo : public QObject
{
    Q_OBJECT

private slots:
    void singleTapHasNoTempo();
    void steadyTaps();
    void averagesLastEightIntervals();
    void resetsAfterLongGap();
    void resetsWhenMovingBackwards();
    void clampsTempo();
    void explicitReset();
};

void TestTapTempo::singleTapHasNoTempo()
{
    TapTempo tap;
    tap.tap(1000.0);
    QCOMPARE(tap.tapCount(), 1);
    QVERIFY(!tap.bpm().has_value());
}

void TestTapTempo::steadyTaps()
{
    TapTempo tap;
    for(const double position : {0.0, 500.0, 1000.0, 1500.0}) {
        tap.tap(position);
    }
    QCOMPARE(tap.tapCount(), 4);
    QCOMPARE(tap.bpm(), std::optional<double>{120.0});
}

void TestTapTempo::averagesLastEightIntervals()
{
    TapTempo tap;
    tap.tap(0.0); // the 1000 ms interval to the next tap falls out of the window
    for(double position = 1000.0; position <= 5000.0; position += 500.0) {
        tap.tap(position);
    }
    QCOMPARE(tap.tapCount(), 10);
    QCOMPARE(tap.bpm(), std::optional<double>{120.0});
}

void TestTapTempo::resetsAfterLongGap()
{
    TapTempo tap;
    tap.tap(0.0);
    tap.tap(500.0);
    tap.tap(2600.0); // 2.1 s gap
    QCOMPARE(tap.tapCount(), 1);
    QVERIFY(!tap.bpm().has_value());
}

void TestTapTempo::resetsWhenMovingBackwards()
{
    TapTempo tap;
    tap.tap(5000.0);
    tap.tap(5500.0);
    tap.tap(1000.0);
    QCOMPARE(tap.tapCount(), 1);
}

void TestTapTempo::clampsTempo()
{
    TapTempo tap;
    tap.tap(0.0);
    tap.tap(100.0); // 600 BPM
    QCOMPARE(tap.bpm(), std::optional<double>{300.0});
}

void TestTapTempo::explicitReset()
{
    TapTempo tap;
    tap.tap(0.0);
    tap.tap(500.0);
    tap.reset();
    QCOMPARE(tap.tapCount(), 0);
    QVERIFY(!tap.bpm().has_value());
}

QTEST_APPLESS_MAIN(TestTapTempo)
#include "tst_taptempo.moc"
