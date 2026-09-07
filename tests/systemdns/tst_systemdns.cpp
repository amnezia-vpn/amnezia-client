#include <QtTest>
#include <QJsonObject>
#include <QSignalSpy>
#include "daemon/daemon.h"
#ifdef Q_OS_WIN
#include "platforms/windows/daemon/dnsutilswindows.h"
#elif defined(Q_OS_MACOS)
#include "platforms/macos/daemon/dnsutilsmacos.h"
#endif

class FakeDns final : public DnsUtils {
public:
    FakeDns() : DnsUtils(nullptr) {}
    QStringList servers { "192.0.2.53", "2001:db8::53" };
    mutable int reads = 0;
    int updates = 0;
    int restores = 0;
    QStringList systemResolvers() const override { ++reads; return servers; }
    bool updateResolvers(const QString&, const QList<QHostAddress>&) override {
        ++updates;
        return true;
    }
    bool restoreResolvers() override { ++restores; return true; }
};

class FakeWireguard final : public WireguardUtils {
public:
    FakeWireguard() : WireguardUtils(nullptr) {}
    bool exists = false;
    bool failRoute = false;
    bool failPeer = false;
    int creates = 0;
    int deletes = 0;
    InterfaceConfig applied;
    QList<IPAddress> exclusions;
    bool interfaceExists() override { return exists; }
    bool addInterface(const InterfaceConfig&) override { ++creates; exists = true; return true; }
    bool deleteInterface() override { ++deletes; exists = false; return true; }
    bool updatePeer(const InterfaceConfig& config) override { applied = config; return !failPeer; }
    bool deletePeer(const InterfaceConfig&) override { return true; }
    QList<PeerStatus> getPeerStatus() override { return {}; }
    bool updateRoutePrefix(const IPAddress&) override { return true; }
    bool deleteRoutePrefix(const IPAddress&) override { return true; }
    bool addExclusionRoute(const IPAddress& ip) override {
        if (failRoute) return false;
        exclusions.append(ip);
        return true;
    }
    bool deleteExclusionRoute(const IPAddress& ip) override { exclusions.removeAll(ip); return true; }
    bool excludeLocalNetworks(const QList<IPAddress>&) override { return true; }
};

class FakeDaemon final : public Daemon {
public:
    FakeDaemon() : Daemon(nullptr) {}
    ~FakeDaemon() override { deactivate(false); }
    FakeDns dns;
    mutable FakeWireguard wg;
protected:
    WireguardUtils* wgutils() const override { return &wg; }
    DnsUtils* dnsutils() override { return &dns; }
};

static InterfaceConfig config(bool systemDns = true) {
    InterfaceConfig result;
    result.m_hopType = InterfaceConfig::SingleHop;
    result.m_privateKey = "test-private-key";
    result.m_serverPublicKey = "test-server-key";
    result.m_deviceIpv4Address = "10.8.0.2";
    result.m_serverIpv4AddrIn = "198.51.100.1";
    result.m_serverPort = 51820;
    result.m_primaryDnsServer = "1.1.1.1";
    result.m_secondaryDnsServer = "1.0.0.1";
    result.m_useSystemDns = systemDns;
    result.m_killSwitchEnabled = true;
    result.m_allowedIPAddressRanges = { IPAddress(QStringLiteral("0.0.0.0/0")), IPAddress(QStringLiteral("::/0")) };
    return result;
}

class SystemDnsTests : public QObject {
    Q_OBJECT
private slots:
    void preservesResolversAndKillSwitch() {
        FakeDaemon daemon;
        auto requested = config();
        requested.m_allowedDnsServers = { "203.0.113.53", "192.0.2.53" };
        QVERIFY(daemon.activate(requested));
        QCOMPARE(daemon.dns.updates, 0);
        QCOMPARE(daemon.dns.reads, 1);
        QVERIFY(daemon.wg.applied.m_killSwitchEnabled);
        QVERIFY(daemon.wg.applied.m_primaryDnsServer.isEmpty());
        QVERIFY(daemon.wg.applied.m_secondaryDnsServer.isEmpty());
        QCOMPARE(daemon.wg.applied.m_allowedDnsServers.size(), 3);
        QVERIFY(daemon.wg.applied.m_allowedDnsServers.contains("203.0.113.53"));
        QVERIFY(daemon.wg.applied.m_allowedDnsServers.contains("2001:db8::53"));
        // DNS exceptions must not grant access to all ports of a DNS server.
        QVERIFY(daemon.wg.applied.m_excludedAddresses.isEmpty());
        QCOMPARE(daemon.wg.exclusions.size(), 2);
        QVERIFY(daemon.deactivate(false));
        QVERIFY(daemon.wg.exclusions.isEmpty());
        QCOMPARE(requested.m_primaryDnsServer, QString("1.1.1.1"));
    }

    void existingModeStillAppliesDns() {
        FakeDaemon daemon;
        QVERIFY(daemon.activate(config(false)));
        QCOMPARE(daemon.dns.reads, 0);
        QCOMPARE(daemon.dns.updates, 1);
        QCOMPARE(daemon.wg.applied.m_primaryDnsServer, QString("1.1.1.1"));
        QVERIFY(daemon.wg.exclusions.isEmpty());
    }

    void rereadsAfterNetworkChange() {
        FakeDaemon daemon;
        QVERIFY(daemon.activate(config()));
        daemon.dns.servers = { "203.0.113.54" };
        QVERIFY(daemon.activate(config()));
        QCOMPARE(daemon.dns.reads, 2);
        QCOMPARE(daemon.wg.creates, 2);
        QCOMPARE(daemon.wg.deletes, 1);
        QCOMPARE(daemon.wg.applied.m_allowedDnsServers, daemon.dns.servers);
        QCOMPARE(daemon.wg.exclusions, QList<IPAddress>{IPAddress(QStringLiteral("203.0.113.54"))});
    }

    void modeChangeRestoresBeforeReading() {
        FakeDaemon daemon;
        QVERIFY(daemon.activate(config(false)));
        QVERIFY(daemon.activate(config()));
        QCOMPARE(daemon.dns.restores, 1);
        QCOMPARE(daemon.dns.updates, 1);
        QVERIFY(daemon.activate(config(false)));
        QCOMPARE(daemon.dns.updates, 2);
        QVERIFY(daemon.wg.exclusions.isEmpty());
        QVERIFY(daemon.wg.applied.m_allowedDnsServers.isEmpty());
    }

    void missingDnsFailsBeforeInterfaceCreation() {
        FakeDaemon daemon;
        daemon.dns.servers.clear();
        QSignalSpy failure(&daemon, &Daemon::activationFailure);
        QVERIFY(!daemon.activate(config()));
        QCOMPARE(failure.size(), 1);
        QCOMPARE(daemon.wg.creates, 0);
        QCOMPARE(daemon.dns.updates, 0);
    }

    void localResolversKeepTheirRoutes() {
        FakeDaemon daemon;
        daemon.dns.servers = { "127.0.0.1", "::1", "fe80::53%3" };
        QVERIFY(daemon.activate(config()));
        QVERIFY(daemon.wg.exclusions.isEmpty());
        QCOMPARE(daemon.wg.applied.m_systemDnsServers, daemon.dns.servers);
        QVERIFY(daemon.wg.applied.m_allowedDnsServers.contains("fe80::53"));
        QVERIFY(!daemon.wg.applied.m_allowedDnsServers.contains("fe80::53%3"));
    }

    void routeFailureCleansUp() {
        FakeDaemon daemon;
        daemon.wg.failRoute = true;
        QVERIFY(!daemon.activate(config()));
        QVERIFY(!daemon.wg.exists);
        QVERIFY(daemon.wg.exclusions.isEmpty());
        QCOMPARE(daemon.dns.updates, 0);
    }

    void peerFailureRemovesDnsRoutes() {
        FakeDaemon daemon;
        daemon.wg.failPeer = true;
        QVERIFY(!daemon.activate(config()));
        QVERIFY(!daemon.wg.exists);
        QVERIFY(daemon.wg.exclusions.isEmpty());
        QCOMPARE(daemon.dns.updates, 0);
    }

    void ipcFlagValidation() {
        QJsonObject json = config().toJson();
        InterfaceConfig parsed;
        QVERIFY(Daemon::parseConfig(json, parsed));
        QVERIFY(parsed.m_useSystemDns);
        json.insert("useSystemDns", "true");
        QVERIFY(!Daemon::parseConfig(json, parsed));
        json.remove("useSystemDns");
        QVERIFY(Daemon::parseConfig(json, parsed));
        QVERIFY(!parsed.m_useSystemDns);
    }

    void wireguardConfigDoesNotOverrideDns() {
        QVERIFY(!config().toWgConf().contains("DNS ="));
        QVERIFY(config(false).toWgConf().contains("DNS = 1.1.1.1, 1.0.0.1"));
    }

    void rejectsInvalidFirewallDns_data() {
        QTest::addColumn<QString>("address");
        QTest::newRow("shell") << "192.0.2.1; echo injected";
        QTest::newRow("network") << "0.0.0.0/0";
        QTest::newRow("unspecified") << "0.0.0.0";
        QTest::newRow("empty") << "";
        QTest::newRow("multicast") << "ff02::1";
        QTest::newRow("hostname") << "dns.example.test";
    }

    void rejectsInvalidFirewallDns() {
        QFETCH(QString, address);
        auto requested = config();
        requested.m_allowedDnsServers = { address };
        InterfaceConfig parsed;
        QVERIFY(!Daemon::parseConfig(requested.toJson(), parsed));
    }

    void readsNativeSystemResolvers() {
        if (!qEnvironmentVariableIsSet("AMNEZIA_TEST_SYSTEM_DNS")) {
            QSKIP("Set AMNEZIA_TEST_SYSTEM_DNS=1 for a read-only native resolver check");
        }
#if defined(Q_OS_WIN)
        DnsUtilsWindows dns(nullptr);
#elif defined(Q_OS_MACOS)
        DnsUtilsMacos dns(nullptr);
#else
        QSKIP("Native resolver discovery is supported on Windows and macOS");
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        const QStringList servers = dns.systemResolvers();
        QVERIFY(!servers.isEmpty());
        for (const QString& server : servers) {
            QVERIFY(!QHostAddress(server).isNull());
        }
        qInfo() << "System DNS:" << servers;
#endif
    }
};

QTEST_GUILESS_MAIN(SystemDnsTests)
#include "tst_systemdns.moc"
