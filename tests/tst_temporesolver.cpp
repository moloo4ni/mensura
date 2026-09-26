#include "temporesolver.h"

#include <core/track.h>

#include <QTest>

using namespace Fooyin;
using namespace Fooyin::Mensura;
using namespace Qt::StringLiterals;

class TestTempoResolver : public QObject
{
    Q_OBJECT

private slots:
    void parsesTag_data();
    void parsesTag();
    void readsTrackTag();
    void resolvesByMode();
};

void TestTempoResolver::parsesTag_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<double>("bpm");

    QTest::newRow("integer") << u"128"_s << true << 128.0;
    QTest::newRow("decimal point") << u"127.9"_s << true << 127.9;
    QTest::newRow("decimal comma") << u"127,9"_s << true << 127.9;
    QTest::newRow("with unit") << u"128 BPM"_s << true << 128.0;
    QTest::newRow("prefixed") << u"BPM: 140.5"_s << true << 140.5;
    QTest::newRow("spaces") << u"  90 "_s << true << 90.0;
    QTest::newRow("too high") << u"1000"_s << true << 300.0;
    QTest::newRow("too low") << u"5"_s << true << 20.0;
    QTest::newRow("zero") << u"0"_s << false << 0.0;
    QTest::newRow("negative") << u"-120"_s << false << 0.0;
    QTest::newRow("negative after space") << u"BPM -120"_s << false << 0.0;
    QTest::newRow("hyphen separator") << u"BPM-128"_s << true << 128.0;
    QTest::newRow("letters") << u"abc"_s << false << 0.0;
    QTest::newRow("empty") << QString{} << false << 0.0;
}

void TestTempoResolver::parsesTag()
{
    QFETCH(QString, text);
    QFETCH(bool, valid);
    QFETCH(double, bpm);

    const auto parsed = parseBpmTag(text);
    QCOMPARE(parsed.has_value(), valid);
    if(valid) {
        QCOMPARE(*parsed, bpm);
    }
}

void TestTempoResolver::readsTrackTag()
{
    Track tagged;
    tagged.addExtraTag(u"BPM"_s, QStringList{u"abc"_s, u"96"_s});
    QCOMPARE(trackBpm(tagged), std::optional<double>{96.0});

    Track lowercase;
    lowercase.addExtraTag(u"bpm"_s, QStringList{u"110"_s});
    QCOMPARE(trackBpm(lowercase), std::optional<double>{110.0});

    const Track untagged;
    QVERIFY(!trackBpm(untagged).has_value());
}

void TestTempoResolver::resolvesByMode()
{
    const auto autoTag = resolveTempo(BpmMode::Auto, 140.0, 120.0);
    QCOMPARE(autoTag.bpm, 140.0);
    QCOMPARE(autoTag.source, BpmSource::Tag);

    const auto autoNoTag = resolveTempo(BpmMode::Auto, std::nullopt, 120.0);
    QCOMPARE(autoNoTag.bpm, 120.0);
    QCOMPARE(autoNoTag.source, BpmSource::Manual);

    const auto manual = resolveTempo(BpmMode::Manual, 140.0, 100.0);
    QCOMPARE(manual.bpm, 100.0);
    QCOMPARE(manual.source, BpmSource::Manual);

    QCOMPARE(resolveTempo(BpmMode::Manual, std::nullopt, 500.0).bpm, 300.0);
}

QTEST_GUILESS_MAIN(TestTempoResolver)
#include "tst_temporesolver.moc"
