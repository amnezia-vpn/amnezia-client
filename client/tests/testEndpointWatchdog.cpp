#include <QTest>

#include "core/utils/endpointWatchdog.h"

namespace
{
    constexpr int64_t kSec = 1000;
    constexpr int64_t kStart = 1'700'000'000'000; // any wall clock value
    constexpr int64_t kStale = EndpointWatchdog::kStaleHandshakeMs;
}

class TestEndpointWatchdog : public QObject
{
    Q_OBJECT

private slots:
    void liveTunnelIsNotStale()
    {
        EndpointWatchdog w;
        w.tunnelStarted(kStart);
        // keepalive every 25 s, handshake renewed every 120 s
        uint64_t tx = 0;
        for (int64_t t = 0; t <= 600 * kSec; t += 20 * kSec) {
            tx += 148;
            const int64_t handshake = kStart + (t / (120 * kSec)) * 120 * kSec + kSec;
            QVERIFY(!w.observe(kStart + t, handshake, tx));
        }
    }

    void sendingWithoutHandshakeIsStale()
    {
        EndpointWatchdog w;
        w.tunnelStarted(kStart);
        const int64_t handshake = kStart + kSec;
        QVERIFY(!w.observe(kStart + 10 * kSec, handshake, 1000));
        QVERIFY(!w.observe(kStart + 100 * kSec, handshake, 2000));
        QVERIFY(!w.observe(handshake + kStale, handshake, 3000));
        QVERIFY(w.observe(handshake + kStale + 20 * kSec, handshake, 4000));
    }

    void neverHandshakedSinceStartIsStale()
    {
        EndpointWatchdog w;
        w.tunnelStarted(kStart);
        QVERIFY(!w.observe(kStart + 20 * kSec, 0, 148));
        QVERIFY(!w.observe(kStart + kStale, 0, 296));
        QVERIFY(w.observe(kStart + kStale + 20 * kSec, 0, 444));
    }

    void idleTunnelWithoutKeepaliveIsNotStale()
    {
        EndpointWatchdog w;
        w.tunnelStarted(kStart);
        const int64_t handshake = kStart + kSec;
        QVERIFY(!w.observe(kStart + 20 * kSec, handshake, 5000));
        // nothing is sent any more: an old handshake is expected, not a failure
        for (int64_t t = 40 * kSec; t <= 3600 * kSec; t += 20 * kSec) {
            QVERIFY(!w.observe(kStart + t, handshake, 5000));
        }
    }

    void refreshIsRepeatedWithGrowingDelay()
    {
        EndpointWatchdog w(3 * 60 * kSec, 10 * 60 * kSec);
        w.tunnelStarted(kStart);
        uint64_t tx = 0;
        int64_t now = kStart;
        auto tick = [&]() {
            now += 20 * kSec;
            tx += 148;
            return w.observe(now, 0, tx);
        };

        while (!tick()) {}
        w.refreshAttempted(now);
        const int64_t first = now;

        while (!tick()) {}
        QCOMPARE(now - first >= 3 * 60 * kSec, true);
        w.refreshAttempted(now);
        const int64_t second = now;

        while (!tick()) {}
        QCOMPARE(now - second >= 6 * 60 * kSec, true);
        w.refreshAttempted(now);
        const int64_t third = now;

        while (!tick()) {}
        QCOMPARE(now - third >= 10 * 60 * kSec, true); // capped
        QCOMPARE(now - third < 12 * 60 * kSec, true);
    }

    void handshakeResetsTheDelay()
    {
        EndpointWatchdog w(3 * 60 * kSec, 30 * 60 * kSec);
        w.tunnelStarted(kStart);
        uint64_t tx = 0;
        int64_t now = kStart;
        auto tick = [&](int64_t handshake) {
            now += 20 * kSec;
            tx += 148;
            return w.observe(now, handshake, tx);
        };

        while (!tick(0)) {}
        w.refreshAttempted(now);
        while (!tick(0)) {}
        w.refreshAttempted(now); // delay is 6 min now

        const int64_t handshake = now + kSec;
        QVERIFY(!tick(handshake));
        const int64_t recovered = now;
        while (!tick(handshake)) {}
        // stale again after the handshake ages, and allowed immediately
        QCOMPARE(now - recovered <= kStale + 40 * kSec, true);
    }

    void counterResetAfterRestartIsNotTraffic()
    {
        EndpointWatchdog w;
        w.tunnelStarted(kStart);
        QVERIFY(!w.observe(kStart + 20 * kSec, kStart + kSec, 900000));

        const int64_t restart = kStart + 60 * kSec;
        w.tunnelStarted(restart);
        // new interface: counters start from zero, the first sample is a baseline
        QVERIFY(!w.observe(restart + 20 * kSec, 0, 0));
        for (int64_t t = 40 * kSec; t <= 600 * kSec; t += 20 * kSec) {
            QVERIFY(!w.observe(restart + t, 0, 0));
        }
    }
};

QTEST_MAIN(TestEndpointWatchdog)
#include "testEndpointWatchdog.moc"
