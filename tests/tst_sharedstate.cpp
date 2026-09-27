#include "sharedstate.h"

#include <QTest>

#include <atomic>
#include <thread>

using namespace Fooyin::Mensura;

namespace {
// Every field is derived from i, so a torn snapshot is detectable.
MensuraParams paramsFor(int64_t i)
{
    MensuraParams params;
    params.enabled     = (i % 2) == 0;
    params.bpm         = 20.0 + static_cast<double>(i % 281);
    params.beatsPerBar = 1 + static_cast<int>(i % 16);
    params.sound       = static_cast<ClickSound>(i % 3);
    params.accent      = static_cast<AccentMode>(i % 3);
    params.gain        = static_cast<double>(i % 101) / 100.0;
    params.phaseNs     = i;
    return params;
}
} // namespace

class TestSharedState : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreDisabled();
    void publishThenRead();
    void invalidSoundFallsBackToClick();
    void invalidAccentFallsBackToPitch();
    void heartbeatRoundTrips();
    void nowIsMonotonic();
    void concurrentSnapshotsAreConsistent();
};

void TestSharedState::defaultsAreDisabled()
{
    const SharedState state;
    MensuraParams params;
    params.enabled = true;
    QVERIFY(state.tryRead(params));
    QVERIFY(params == MensuraParams{});
    QCOMPARE(state.revision(), uint64_t{0});
    QCOMPARE(state.lastHeartbeatNs(), int64_t{0});
}

void TestSharedState::publishThenRead()
{
    SharedState state;
    const MensuraParams published = paramsFor(42);
    state.publish(published);

    MensuraParams read;
    QVERIFY(state.tryRead(read));
    QVERIFY(read == published);
    QCOMPARE(state.revision(), uint64_t{1});
}

void TestSharedState::invalidSoundFallsBackToClick()
{
    SharedState state;
    MensuraParams published;
    published.sound = static_cast<ClickSound>(9);
    state.publish(published);

    MensuraParams read;
    QVERIFY(state.tryRead(read));
    QCOMPARE(read.sound, ClickSound::Click);
}

void TestSharedState::invalidAccentFallsBackToPitch()
{
    SharedState state;
    MensuraParams published;
    published.accent = static_cast<AccentMode>(9);
    state.publish(published);

    MensuraParams read;
    QVERIFY(state.tryRead(read));
    QCOMPARE(read.accent, AccentMode::Pitch);
}

void TestSharedState::heartbeatRoundTrips()
{
    SharedState state;
    state.heartbeat(123'456);
    QCOMPARE(state.lastHeartbeatNs(), int64_t{123'456});
}

void TestSharedState::nowIsMonotonic()
{
    const int64_t first  = SharedState::nowNs();
    const int64_t second = SharedState::nowNs();
    QVERIFY(first > 0);
    QVERIFY(second >= first);
}

void TestSharedState::concurrentSnapshotsAreConsistent()
{
    constexpr int64_t Iterations = 200'000;

    SharedState state;
    std::atomic<bool> done{false};

    std::thread writer{[&] {
        for(int64_t i = 1; i <= Iterations; ++i) {
            state.publish(paramsFor(i));
        }
        done.store(true);
    }};

    int64_t reads{0};
    bool consistent{true};
    while(!done.load()) {
        MensuraParams params;
        if(state.tryRead(params)) {
            ++reads;
            if(params.phaseNs != 0 && !(params == paramsFor(params.phaseNs))) {
                consistent = false;
            }
        }
    }
    writer.join();

    QVERIFY(consistent);
    QVERIFY(reads > 0);

    MensuraParams last;
    QVERIFY(state.tryRead(last));
    QVERIFY(last == paramsFor(Iterations));
    QCOMPARE(state.revision(), static_cast<uint64_t>(Iterations));
}

QTEST_APPLESS_MAIN(TestSharedState)
#include "tst_sharedstate.moc"
