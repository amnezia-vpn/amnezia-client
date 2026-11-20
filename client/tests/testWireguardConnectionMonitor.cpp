#include <QTest>

#include "daemon/wireguardconnectionmonitor.h"

class TestWireguardConnectionMonitor : public QObject
{
    Q_OBJECT

    using Action = WireguardConnectionMonitor::Action;

private slots:
    void requiresInitialHandshake()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(0, 0, 100, 0), Action::None);
        QCOMPARE(monitor.unavailable(1000000), Action::None);
        QCOMPARE(monitor.unavailable(2000000), Action::None);
        QCOMPARE(monitor.update(0, 100, 1000, 3000000), Action::None);
        QVERIFY(!monitor.isRecovering());
        QCOMPARE(monitor.update(1, 200, 2000, 4000000), Action::None);
        QCOMPARE(monitor.update(1, 200, 3000, 4000001), Action::None);
        QVERIFY(!monitor.isRecovering());
    }

    void idlePeerDoesNotRecover()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 180000), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 3600000), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 86400000), Action::None);
        QVERIFY(!monitor.isRecovering());
    }

    void expiryBoundaryDoesNotCountEarlierTraffic()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 180000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 240000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 3600000), Action::None);
        QVERIFY(!monitor.isRecovering());
    }

    void newTrafficAfterIdleGetsFullGrace()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 180000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 3600000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 3629999), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 3630000), Action::Rebind);
        QVERIFY(monitor.isRecovering());
    }

    void receivedPacketCancelsSuspicion()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 180000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 181000), Action::None);
        QCOMPARE(monitor.update(1, 101, 200, 210000), Action::None);
        QCOMPARE(monitor.update(1, 101, 200, 1000000), Action::None);
        QVERIFY(!monitor.isRecovering());
    }

    void refreshedHandshakeRestartsKeyLifetime()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 180000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 181000), Action::None);
        QCOMPARE(monitor.update(2, 100, 200, 200000), Action::None);
        QCOMPARE(monitor.update(2, 100, 300, 211000), Action::None);
        QCOMPARE(monitor.update(2, 100, 400, 380000), Action::None);
        QCOMPARE(monitor.update(2, 100, 500, 381000), Action::None);
        QCOMPARE(monitor.update(2, 100, 500, 411000), Action::Rebind);
    }

    void repeatedRebindsUseCappedBackoff()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 180000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 181000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 210999), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 211000), Action::Rebind);
        const QList<qint64> retries { 271000, 391000, 631000, 931000, 1231000 };
        for (qint64 retry : retries) {
            QCOMPARE(monitor.update(1, 100, 200, retry - 1), Action::None);
            QCOMPARE(monitor.update(1, 100, 200, retry), Action::Rebind);
            QVERIFY(monitor.isRecovering());
        }
    }

    void recoveryRequiresPeerResponse_data()
    {
        QTest::addColumn<qint64>("handshake");
        QTest::addColumn<qint64>("rxBytes");
        QTest::newRow("received-packet") << qint64(1) << qint64(101);
        QTest::newRow("new-handshake") << qint64(2) << qint64(100);
    }

    void recoveryRequiresPeerResponse()
    {
        QFETCH(qint64, handshake);
        QFETCH(qint64, rxBytes);
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.unavailable(1000), Action::None);
        QCOMPARE(monitor.unavailable(31000), Action::Rebind);
        QCOMPARE(monitor.update(1, 100, 200, 32000), Action::None);
        QVERIFY(monitor.isRecovering());
        QCOMPARE(monitor.update(handshake, rxBytes, 200, 33000), Action::Recovered);
        QVERIFY(!monitor.isRecovering());
        QCOMPARE(monitor.update(handshake, rxBytes, 200, 34000), Action::None);
    }

    void peerResponseResetsBackoff()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.unavailable(1000), Action::None);
        QCOMPARE(monitor.unavailable(31000), Action::Rebind);
        QCOMPARE(monitor.unavailable(91000), Action::Rebind);
        QCOMPARE(monitor.unavailable(211000), Action::Rebind);
        QCOMPARE(monitor.update(1, 101, 100, 212000), Action::Recovered);
        QCOMPARE(monitor.unavailable(213000), Action::None);
        QCOMPARE(monitor.unavailable(242999), Action::None);
        QCOMPARE(monitor.unavailable(243000), Action::Rebind);
        QCOMPARE(monitor.unavailable(302999), Action::None);
        QCOMPARE(monitor.unavailable(303000), Action::Rebind);
    }

    void counterResetDoesNotConfirmRecovery()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 180000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 181000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 211000), Action::Rebind);
        QCOMPARE(monitor.update(1, 1, 1, 212000), Action::None);
        QVERIFY(monitor.isRecovering());
        QCOMPARE(monitor.update(1, 1, 1, 270999), Action::None);
        QCOMPARE(monitor.update(1, 1, 1, 271000), Action::Rebind);
        QCOMPARE(monitor.update(1, 2, 1, 272000), Action::Recovered);
    }

    void counterResetDiscardsUnansweredBaseline()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 180000), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, 181000), Action::None);
        QCOMPARE(monitor.update(1, 1, 1, 200000), Action::None);
        QCOMPARE(monitor.update(1, 1, 1, 1000000), Action::None);
        QCOMPARE(monitor.update(1, 1, 2, 1001000), Action::None);
        QCOMPARE(monitor.update(1, 1, 2, 1031000), Action::Rebind);
    }

    void unavailablePeerGetsFullGrace()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.unavailable(1000), Action::None);
        QCOMPARE(monitor.unavailable(30999), Action::None);
        QCOMPARE(monitor.unavailable(31000), Action::Rebind);
        QVERIFY(monitor.isRecovering());
    }

    void temporaryStatusFailureDoesNotReconnectIdlePeer()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.unavailable(1000), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 2000), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 31000), Action::None);
        QCOMPARE(monitor.update(1, 100, 100, 1000000), Action::None);
        QVERIFY(!monitor.isRecovering());
        QCOMPARE(monitor.unavailable(1001000), Action::None);
        QCOMPARE(monitor.unavailable(1030999), Action::None);
        QCOMPARE(monitor.unavailable(1031000), Action::Rebind);
    }

    void lostHandshakeAfterEstablishmentTriggersRecovery()
    {
        WireguardConnectionMonitor monitor;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(0, 0, 0, 1000), Action::None);
        QCOMPARE(monitor.update(0, 0, 0, 30999), Action::None);
        QCOMPARE(monitor.update(0, 0, 0, 31000), Action::Rebind);
        QCOMPARE(monitor.update(2, 0, 0, 32000), Action::Recovered);
    }

    void timingRanges_data()
    {
        QTest::addColumn<QString>("rejectAfterTime");
        QTest::addColumn<QString>("rekeyTimeout");
        QTest::addColumn<qint64>("keyLifetimeMsec");
        QTest::addColumn<qint64>("responseTimeoutMsec");
        QTest::newRow("defaults") << QString() << QString() << qint64(180000) << qint64(30000);
        QTest::newRow("scalar") << QString("7") << QString("15") << qint64(7000) << qint64(48000);
        QTest::newRow("ranges") << QString("150-180") << QString("4-8") << qint64(180000) << qint64(30000);
        QTest::newRow("extended-ranges") << QString("300-600") << QString("10-20") << qint64(600000) << qint64(63000);
        QTest::newRow("zero-defaults") << QString("0") << QString("0-0") << qint64(180000) << qint64(30000);
        QTest::newRow("zero-lower-bound") << QString("0-10") << QString("0-20") << qint64(10000) << qint64(63000);
        QTest::newRow("outer-whitespace") << QString(" 150-180 ") << QString(" 4-8 ") << qint64(180000) << qint64(30000);
        const QStringList invalid {
            "-1", "20-10", "x", "1-2-3", "1-", "-", "+5", "1 - 2", "4294967296", "1-4294967296", "18446744073709551616"
        };
        for (const QString &value : invalid) {
            QTest::newRow(qPrintable(value)) << value << value << qint64(180000) << qint64(30000);
        }
        QTest::newRow("maximum-uint32") << QString("1-4294967295") << QString("4294967295") << qint64(4294967295000LL)
                                        << qint64(12884901888000LL);
    }

    void timingRanges()
    {
        QFETCH(QString, rejectAfterTime);
        QFETCH(QString, rekeyTimeout);
        QFETCH(qint64, keyLifetimeMsec);
        QFETCH(qint64, responseTimeoutMsec);
        WireguardConnectionMonitor monitor(rejectAfterTime, rekeyTimeout);
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.update(1, 100, 200, keyLifetimeMsec - 1), Action::None);
        QCOMPARE(monitor.update(1, 100, 300, keyLifetimeMsec), Action::None);
        QCOMPARE(monitor.update(1, 100, 400, keyLifetimeMsec + 1), Action::None);
        QCOMPARE(monitor.update(1, 100, 400, keyLifetimeMsec + responseTimeoutMsec), Action::None);
        QCOMPARE(monitor.update(1, 100, 400, keyLifetimeMsec + responseTimeoutMsec + 1), Action::Rebind);
    }

    void customGraceIsNotShortenedByBackoffCap()
    {
        WireguardConnectionMonitor monitor("1", "1000");
        const qint64 grace = 3003000;
        QCOMPARE(monitor.update(1, 100, 100, 0), Action::None);
        QCOMPARE(monitor.unavailable(1), Action::None);
        QCOMPARE(monitor.unavailable(grace), Action::None);
        QCOMPARE(monitor.unavailable(grace + 1), Action::Rebind);
        QCOMPARE(monitor.unavailable(grace * 2), Action::None);
        QCOMPARE(monitor.unavailable(grace * 2 + 1), Action::Rebind);
    }
};

QTEST_APPLESS_MAIN(TestWireguardConnectionMonitor)

#include "testWireguardConnectionMonitor.moc"
