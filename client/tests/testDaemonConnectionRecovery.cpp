#include <QJsonObject>
#include <QSignalSpy>
#include <QtTest>

#include "daemon/daemon.h"

class RecoveryWireguardUtils : public WireguardUtils
{
public:
    RecoveryWireguardUtils() : WireguardUtils(nullptr)
    {
    }

    bool recoverySupported = true;
    bool interfacePresent = false;
    bool rebindResult = true;
    int rebindCount = 0;
    int interfaceDeleteCount = 0;
    int peerDeleteCount = 0;
    QList<PeerStatus> peers;

    bool interfaceExists() override
    {
        return interfacePresent;
    }
    bool addInterface(const InterfaceConfig &) override
    {
        interfacePresent = true;
        return true;
    }
    bool deleteInterface() override
    {
        ++interfaceDeleteCount;
        interfacePresent = false;
        return true;
    }
    bool updatePeer(const InterfaceConfig &) override
    {
        return true;
    }
    bool deletePeer(const InterfaceConfig &) override
    {
        ++peerDeleteCount;
        return true;
    }
    QList<PeerStatus> getPeerStatus() override
    {
        return peers;
    }
    bool supportsConnectionRecovery() const override
    {
        return recoverySupported;
    }
    bool rebindInterface() override
    {
        ++rebindCount;
        return rebindResult;
    }
    bool updateRoutePrefix(const IPAddress &) override
    {
        return true;
    }
    bool deleteRoutePrefix(const IPAddress &) override
    {
        return true;
    }
    bool addExclusionRoute(const IPAddress &) override
    {
        return true;
    }
    bool deleteExclusionRoute(const IPAddress &) override
    {
        return true;
    }
    bool excludeLocalNetworks(const QList<IPAddress> &) override
    {
        return true;
    }
};

class RecoveryDnsUtils : public DnsUtils
{
public:
    RecoveryDnsUtils() : DnsUtils(nullptr)
    {
    }
    int restoreCount = 0;
    bool updateResolvers(const QString &, const QList<QHostAddress> &) override
    {
        return true;
    }
    bool restoreResolvers() override
    {
        ++restoreCount;
        return true;
    }
};

class RecoveryDaemon : public Daemon
{
public:
    RecoveryDaemon() : Daemon(nullptr)
    {
    }

    mutable RecoveryWireguardUtils backend;
    RecoveryDnsUtils dns;
    qint64 nowMsec = 0;

    using Daemon::checkHandshake;

    bool beginConnection()
    {
        InterfaceConfig config;
        config.m_hopType = InterfaceConfig::SingleHop;
        config.m_killSwitchEnabled = true;
        config.m_serverPublicKey = QStringLiteral("test-peer");
        config.m_primaryDnsServer = QStringLiteral("192.0.2.1");
        config.m_serverIpv4Gateway = QStringLiteral("192.0.2.1");
        config.m_deviceIpv4Address = QStringLiteral("192.0.2.2");

        WireguardUtils::PeerStatus peer(config.m_serverPublicKey);
        peer.m_handshake = 1000;
        peer.m_rxBytes = 100;
        peer.m_txBytes = 100;
        backend.peers = { peer };
        return activate(config);
    }

    void pollAt(qint64 timestamp)
    {
        nowMsec = timestamp;
        m_handshakeTimer.stop();
        checkHandshake();
    }

    bool monitoringActive() const
    {
        return m_handshakeTimer.isActive();
    }

protected:
    WireguardUtils *wgutils() const override
    {
        return &backend;
    }
    DnsUtils *dnsutils() override
    {
        return &dns;
    }
    qint64 monotonicMsec() const override
    {
        return nowMsec;
    }
};

class TestDaemonConnectionRecovery : public QObject
{
    Q_OBJECT

private slots:
    void initialHandshakePending()
    {
        RecoveryDaemon daemon;
        QSignalSpy connected(&daemon, &Daemon::connected);
        QSignalSpy reconnecting(&daemon, &Daemon::reconnecting);
        QVERIFY(daemon.beginConnection());
        daemon.backend.peers.first().m_handshake = 0;
        daemon.backend.peers.first().m_rxBytes = 0;

        QVERIFY(!daemon.getStatus().value("connected").toBool());
        QVERIFY(daemon.getStatus().value("connecting").toBool());
        QVERIFY(!daemon.getStatus().value("reconnecting").toBool());
        daemon.pollAt(0);
        daemon.pollAt(1000000);

        QCOMPARE(connected.count(), 0);
        QCOMPARE(reconnecting.count(), 0);
        QCOMPARE(daemon.backend.rebindCount, 0);
        QVERIFY(daemon.monitoringActive());

        daemon.backend.peers.first().m_handshake = 1000;
        daemon.backend.peers.first().m_rxBytes = 100;
        daemon.pollAt(1000001);
        QCOMPARE(connected.count(), 1);
        QVERIFY(daemon.getStatus().value("connected").toBool());
        QVERIFY(!daemon.getStatus().value("connecting").toBool());
    }

    void continuesMonitoringAfterFirstHandshake()
    {
        RecoveryDaemon daemon;
        QSignalSpy connected(&daemon, &Daemon::connected);
        QVERIFY(daemon.beginConnection());
        daemon.pollAt(0);

        QCOMPARE(connected.count(), 1);
        QVERIFY(daemon.monitoringActive());
        QVERIFY(daemon.getStatus().value("connected").toBool());

        daemon.pollAt(5000);
        QCOMPARE(connected.count(), 1);
        QVERIFY(daemon.monitoringActive());
    }

    void recoversWithoutTearingDownInterfaceOrDns()
    {
        RecoveryDaemon daemon;
        QSignalSpy connected(&daemon, &Daemon::connected);
        QSignalSpy reconnecting(&daemon, &Daemon::reconnecting);
        QSignalSpy disconnected(&daemon, &Daemon::disconnected);
        QVERIFY(daemon.beginConnection());
        daemon.pollAt(0);
        daemon.pollAt(180000);
        daemon.backend.peers.first().m_txBytes += 100;
        daemon.pollAt(181000);
        daemon.pollAt(210999);
        QCOMPARE(daemon.backend.rebindCount, 0);
        daemon.pollAt(211000);

        QCOMPARE(daemon.backend.rebindCount, 1);
        QCOMPARE(reconnecting.count(), 1);
        QCOMPARE(connected.count(), 1);
        QCOMPARE(disconnected.count(), 0);
        QCOMPARE(daemon.backend.interfaceDeleteCount, 0);
        QCOMPARE(daemon.backend.peerDeleteCount, 0);
        QCOMPARE(daemon.dns.restoreCount, 0);
        QVERIFY(daemon.backend.interfacePresent);
        QVERIFY(!daemon.getStatus().value("connected").toBool());
        QVERIFY(daemon.getStatus().value("reconnecting").toBool());

        daemon.backend.peers.first().m_rxBytes += 100;
        daemon.pollAt(212000);

        QCOMPARE(connected.count(), 2);
        QCOMPARE(reconnecting.count(), 1);
        QCOMPARE(daemon.backend.rebindCount, 1);
        QVERIFY(daemon.getStatus().value("connected").toBool());
        QVERIFY(!daemon.getStatus().value("reconnecting").toBool());
    }

    void waitsForMissingPeerGrace()
    {
        RecoveryDaemon daemon;
        QSignalSpy reconnecting(&daemon, &Daemon::reconnecting);
        QVERIFY(daemon.beginConnection());
        daemon.pollAt(0);
        daemon.backend.peers.clear();

        daemon.pollAt(1000);
        const QJsonObject pendingStatus = daemon.getStatus();
        QVERIFY(pendingStatus.value("statusUnavailable").toBool());
        QVERIFY(!pendingStatus.value("reconnecting").toBool());
        daemon.pollAt(30999);
        QCOMPARE(daemon.backend.rebindCount, 0);
        QCOMPARE(reconnecting.count(), 0);
        daemon.pollAt(31000);

        QCOMPARE(daemon.backend.rebindCount, 1);
        QCOMPARE(reconnecting.count(), 1);
        QCOMPARE(daemon.backend.interfaceDeleteCount, 0);
        QVERIFY(daemon.getStatus().value("reconnecting").toBool());
    }

    void failedRebindDoesNotReportConnected()
    {
        RecoveryDaemon daemon;
        daemon.backend.rebindResult = false;
        QSignalSpy connected(&daemon, &Daemon::connected);
        QSignalSpy reconnecting(&daemon, &Daemon::reconnecting);
        QSignalSpy disconnected(&daemon, &Daemon::disconnected);
        QVERIFY(daemon.beginConnection());
        daemon.pollAt(0);
        auto peer = daemon.backend.peers.first();
        daemon.backend.peers.clear();
        daemon.pollAt(1000);
        daemon.pollAt(31000);

        QCOMPARE(daemon.backend.rebindCount, 1);
        QCOMPARE(connected.count(), 1);
        QCOMPARE(reconnecting.count(), 1);
        QVERIFY(!daemon.getStatus().value("connected").toBool());
        QVERIFY(daemon.getStatus().value("reconnecting").toBool());
        daemon.pollAt(90999);
        QCOMPARE(daemon.backend.rebindCount, 1);
        daemon.pollAt(91000);
        QCOMPARE(daemon.backend.rebindCount, 2);
        QCOMPARE(connected.count(), 1);
        QCOMPARE(reconnecting.count(), 1);
        QCOMPARE(disconnected.count(), 0);
        QCOMPARE(daemon.backend.interfaceDeleteCount, 0);
        QCOMPARE(daemon.dns.restoreCount, 0);

        peer.m_rxBytes += 100;
        daemon.backend.peers = { peer };
        daemon.pollAt(92000);
        QCOMPARE(connected.count(), 2);
        QCOMPARE(daemon.backend.rebindCount, 2);
        QVERIFY(daemon.getStatus().value("connected").toBool());
        QVERIFY(!daemon.getStatus().value("reconnecting").toBool());
    }

    void deactivationCancelsMonitoring()
    {
        RecoveryDaemon daemon;
        QSignalSpy reconnecting(&daemon, &Daemon::reconnecting);
        QVERIFY(daemon.beginConnection());
        daemon.pollAt(0);
        daemon.backend.peers.clear();
        daemon.pollAt(1000);
        daemon.pollAt(31000);
        QCOMPARE(daemon.backend.rebindCount, 1);

        QVERIFY(daemon.deactivate());
        QVERIFY(!daemon.monitoringActive());
        QVERIFY(!daemon.getStatus().value("connected").toBool());
        daemon.pollAt(1000000);

        QCOMPARE(daemon.backend.rebindCount, 1);
        QCOMPARE(reconnecting.count(), 1);
        QVERIFY(!daemon.monitoringActive());
    }

    void doesNotEnableRecoveryForUnsupportedBackends()
    {
        RecoveryDaemon daemon;
        daemon.backend.recoverySupported = false;
        QSignalSpy connected(&daemon, &Daemon::connected);
        QSignalSpy reconnecting(&daemon, &Daemon::reconnecting);
        QVERIFY(daemon.beginConnection());
        daemon.pollAt(0);

        QCOMPARE(connected.count(), 1);
        QVERIFY(!daemon.monitoringActive());
        daemon.backend.peers.first().m_txBytes += 100;
        daemon.pollAt(1000000);

        QCOMPARE(daemon.backend.rebindCount, 0);
        QCOMPARE(reconnecting.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestDaemonConnectionRecovery)
#include "testDaemonConnectionRecovery.moc"
