#include "mensuraconfig.h"

#include <QTest>

#include <cmath>

using namespace Fooyin::Mensura;
using namespace Qt::StringLiterals;

class TestMensuraConfig : public QObject
{
    Q_OBJECT

private slots:
    void defaultsMatchSpec();
    void roundTrips();
    void clampsOutOfRange();
    void invalidEnumsFallBack();
    void parsesStrings();
    void garbageFallsBack();
    void convertsDecibels();
};

void TestMensuraConfig::defaultsMatchSpec()
{
    const MensuraConfig config = MensuraConfig::fromMap({});
    QCOMPARE(config.enabled, false);
    QCOMPARE(config.mode, BpmMode::Auto);
    QCOMPARE(config.manualBpm, 120.0);
    QCOMPARE(config.beatsPerBar, 4);
    QCOMPARE(config.sound, ClickSound::Click);
    QCOMPARE(config.volumeDb, -6.0);
    QCOMPARE(config.phaseOffsetMs, 0);
    QVERIFY(config == MensuraConfig{});
}

void TestMensuraConfig::roundTrips()
{
    MensuraConfig config;
    config.enabled       = true;
    config.mode          = BpmMode::Manual;
    config.manualBpm     = 97.5;
    config.beatsPerBar   = 7;
    config.sound         = ClickSound::Beep;
    config.volumeDb      = -18.0;
    config.phaseOffsetMs = -120;

    const QVariantMap map = config.toMap();
    QCOMPARE(map.value(u"Version"_s).toInt(), 1);
    QVERIFY(MensuraConfig::fromMap(map) == config);
}

void TestMensuraConfig::clampsOutOfRange()
{
    const MensuraConfig high = MensuraConfig::fromMap({{u"ManualBpm"_s, 999.0},
                                                       {u"BeatsPerBar"_s, 40},
                                                       {u"VolumeDb"_s, 12.0},
                                                       {u"PhaseOffsetMs"_s, 5000}});
    QCOMPARE(high.manualBpm, 300.0);
    QCOMPARE(high.beatsPerBar, 16);
    QCOMPARE(high.volumeDb, 0.0);
    QCOMPARE(high.phaseOffsetMs, 1000);

    const MensuraConfig low = MensuraConfig::fromMap({{u"ManualBpm"_s, 1.0},
                                                      {u"BeatsPerBar"_s, 0},
                                                      {u"VolumeDb"_s, -100.0},
                                                      {u"PhaseOffsetMs"_s, -1e12}});
    QCOMPARE(low.manualBpm, 20.0);
    QCOMPARE(low.beatsPerBar, 1);
    QCOMPARE(low.volumeDb, -40.0);
    QCOMPARE(low.phaseOffsetMs, -1000);
}

void TestMensuraConfig::invalidEnumsFallBack()
{
    const MensuraConfig config = MensuraConfig::fromMap({{u"Mode"_s, 7}, {u"Sound"_s, 9}});
    QCOMPARE(config.mode, BpmMode::Auto);
    QCOMPARE(config.sound, ClickSound::Click);

    QCOMPARE(MensuraConfig::fromMap({{u"Sound"_s, 1.5}}).sound, ClickSound::Click);
}

void TestMensuraConfig::parsesStrings()
{
    const MensuraConfig config
        = MensuraConfig::fromMap({{u"Enabled"_s, u"true"_s}, {u"ManualBpm"_s, u"90.5"_s}, {u"Sound"_s, u"2"_s}});
    QCOMPARE(config.enabled, true);
    QCOMPARE(config.manualBpm, 90.5);
    QCOMPARE(config.sound, ClickSound::Beep);
}

void TestMensuraConfig::garbageFallsBack()
{
    const MensuraConfig config = MensuraConfig::fromMap({{u"ManualBpm"_s, u"abc"_s}, {u"VolumeDb"_s, QVariantList{}}});
    QCOMPARE(config.manualBpm, 120.0);
    QCOMPARE(config.volumeDb, -6.0);
}

void TestMensuraConfig::convertsDecibels()
{
    QCOMPARE(dbToGain(0.0), 1.0);
    QVERIFY(std::abs(dbToGain(-6.0) - 0.501187) < 1e-6);
    QVERIFY(std::abs(dbToGain(-40.0) - 0.01) < 1e-12);
}

QTEST_GUILESS_MAIN(TestMensuraConfig)
#include "tst_mensuraconfig.moc"
