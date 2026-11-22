#include <QSet>
#include <QSignalSpy>
#include <QTest>

#include "daemon/daemon.h"

class TestWireguardUtils : public WireguardUtils
{
public:
    TestWireguardUtils() : WireguardUtils(nullptr)
    {
    }

    QSet<IPAddress> routes;
    QSet<IPAddress> exclusions;
    InterfaceConfig peer;
    QString failingRoute;
    bool partialRouteFailure = false;
    bool unknownRouteCreation = false;
    bool expandDefaultRoutes = false;
    QString failingRemoval;
    bool failNextPeerUpdate = false;
    bool rejectNextPeerUpdate = false;
    bool failAllPeerUpdates = false;
    bool failExclusionAdd = false;
    bool failExclusionRemoval = false;
    int interfaceChanges = 0;
    int peerUpdates = 0;
    int initialPeerUpdates = 0;
    int livePeerUpdates = 0;

    bool interfaceExists() override
    {
        return true;
    }
    bool addInterface(const InterfaceConfig &) override
    {
        ++interfaceChanges;
        return true;
    }
    bool deleteInterface() override
    {
        ++interfaceChanges;
        return true;
    }
    bool updatePeer(const InterfaceConfig &config) override
    {
        ++initialPeerUpdates;
        return applyConfiguration(config);
    }
    SplitTunnelingUpdateResult updateSplitTunneling(const InterfaceConfig &config) override
    {
        ++livePeerUpdates;
        if (rejectNextPeerUpdate) {
            rejectNextPeerUpdate = false;
            return SplitTunnelingUpdateResult::NotApplied;
        }
        if (failAllPeerUpdates) {
            return SplitTunnelingUpdateResult::StateUnknown;
        }
        return applyConfiguration(config) ? SplitTunnelingUpdateResult::Applied
                                          : SplitTunnelingUpdateResult::StateUnknown;
    }
    SplitTunnelingUpdateResult restoreSplitTunneling(const InterfaceConfig &config) override
    {
        return updateSplitTunneling(config);
    }
    bool applyConfiguration(const InterfaceConfig &config)
    {
        ++peerUpdates;
        if (failNextPeerUpdate) {
            failNextPeerUpdate = false;
            return false;
        }
        peer = config;
        return true;
    }
    bool deletePeer(const InterfaceConfig &) override
    {
        return true;
    }
    QList<PeerStatus> getPeerStatus() override
    {
        return { };
    }
    RouteCreationResult createRoutePrefix(const IPAddress &prefix) override
    {
        if (unknownRouteCreation) {
            return { SplitTunnelingUpdateResult::StateUnknown, { } };
        }
        if (expandDefaultRoutes && prefix.prefixLength() == 0) {
            return createTunnelRoutes(prefix, [&](const auto &route) {
                if (routes.contains(route))
                    return RouteMutationResult::Unchanged;
                routes.insert(route);
                return RouteMutationResult::Applied;
            });
        }
        if (routes.contains(prefix)) {
            return { };
        }
        const bool result = updateRoutePrefix(prefix);
        return { result ? SplitTunnelingUpdateResult::Applied : SplitTunnelingUpdateResult::NotApplied,
                 routes.contains(prefix) ? QList<IPAddress> { prefix } : QList<IPAddress> { } };
    }
    RouteCreationResult createExclusionRoute(const IPAddress &prefix) override
    {
        if (exclusions.contains(prefix))
            return { };
        const bool result = addExclusionRoute(prefix);
        return { result ? SplitTunnelingUpdateResult::Applied : SplitTunnelingUpdateResult::NotApplied, { prefix } };
    }
    bool updateRoutePrefix(const IPAddress &prefix) override
    {
        if (prefix.toString() == failingRoute) {
            if (partialRouteFailure) {
                routes.insert(prefix);
            }
            return false;
        }
        routes.insert(prefix);
        return true;
    }
    bool deleteRoutePrefix(const IPAddress &prefix) override
    {
        if (prefix.toString() == failingRemoval)
            return false;
        routes.remove(prefix);
        return true;
    }
    bool addExclusionRoute(const IPAddress &prefix) override
    {
        exclusions.insert(prefix);
        return !failExclusionAdd;
    }
    bool deleteExclusionRoute(const IPAddress &prefix) override
    {
        exclusions.remove(prefix);
        return !failExclusionRemoval;
    }
    bool excludeLocalNetworks(const QList<IPAddress> &) override
    {
        return true;
    }
};

class TestSplitTunnelDaemon : public Daemon
{
public:
    TestSplitTunnelDaemon() : Daemon(nullptr)
    {
    }
    mutable TestWireguardUtils backend;

    void seed(const InterfaceConfig &config)
    {
        m_connections[config.m_hopType] = ConnectionState(config);
        m_connections[config.m_hopType].m_date = QDateTime::fromMSecsSinceEpoch(123456);
        backend.peer = config;
        for (const auto &route : config.m_allowedIPAddressRanges)
            backend.routes.insert(route);
        for (const auto &route : config.m_excludedAddresses) {
            backend.exclusions.insert(IPAddress(route));
            m_excludedAddrSet[IPAddress(route)] = 1;
        }
    }
    InterfaceConfig configuration() const
    {
        return m_connections.first().m_config;
    }
    QDateTime connectedAt() const
    {
        return m_connections.first().m_date;
    }

protected:
    WireguardUtils *wgutils() const override
    {
        return &backend;
    }
};

class TestSplitTunnelingDaemon : public QObject
{
    Q_OBJECT

    static InterfaceConfig initialConfiguration()
    {
        InterfaceConfig config;
        config.m_hopType = InterfaceConfig::SingleHop;
        config.m_serverPublicKey = "test-peer";
        config.m_splitTunnelSessionId = "test-session";
        config.m_serverIpv4AddrIn = "203.0.113.1";
        config.m_privateKey = "test-private-key";
        config.m_junkPacketCount = "4";
        config.m_killSwitchEnabled = true;
        config.m_allowedIPAddressRanges = { IPAddress("192.0.2.1/32") };
        config.m_excludedAddresses = { config.m_serverIpv4AddrIn };
        return config;
    }

private slots:
    void inclusionUpdatesPeerAndRoutesWithoutRestart()
    {
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        QSignalSpy disconnected(&daemon, &Daemon::disconnected);
        const auto connectedAt = daemon.connectedAt();
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.0/24" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::Applied);
        QVERIFY(daemon.backend.routes.contains(IPAddress("198.51.100.0/24")));
        QVERIFY(daemon.backend.peer.m_allowedIPAddressRanges.contains(IPAddress("198.51.100.0/24")));
        QCOMPARE(daemon.backend.peer.m_privateKey, initialConfiguration().m_privateKey);
        QCOMPARE(daemon.backend.peer.m_junkPacketCount, initialConfiguration().m_junkPacketCount);
        QVERIFY(daemon.backend.peer.m_killSwitchEnabled);
        QCOMPARE(daemon.connectedAt(), connectedAt);
        QCOMPARE(daemon.backend.interfaceChanges, 0);
        QCOMPARE(disconnected.count(), 0);
        QCOMPARE(daemon.backend.initialPeerUpdates, 0);
        QCOMPARE(daemon.backend.livePeerUpdates, 1);
    }

    void exclusionKeepsFullTunnelAndServerRoute()
    {
        TestSplitTunnelDaemon daemon;
        auto config = initialConfiguration();
        config.m_allowedIPAddressRanges = { IPAddress("0.0.0.0/0"), IPAddress("::/0") };
        daemon.seed(config);
        QCOMPARE(daemon.updateSplitTunneling(2, { "198.51.100.0/24" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::Applied);
        QVERIFY(daemon.backend.routes.contains(IPAddress("0.0.0.0/0")));
        QVERIFY(daemon.backend.exclusions.contains(IPAddress("198.51.100.0/24")));
        QVERIFY(daemon.backend.exclusions.contains(IPAddress("203.0.113.1/32")));
        QCOMPARE(daemon.backend.interfaceChanges, 0);
    }

    void emptyListCanBecomeAnInclusionList()
    {
        TestSplitTunnelDaemon daemon;
        auto config = initialConfiguration();
        config.m_allowedIPAddressRanges = { IPAddress("0.0.0.0/0"), IPAddress("::/0") };
        daemon.seed(config);
        QCOMPARE(daemon.updateSplitTunneling(1, { "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::Applied);
        QVERIFY(!daemon.backend.routes.contains(IPAddress("0.0.0.0/0")));
        QVERIFY(daemon.backend.routes.contains(IPAddress("198.51.100.1/32")));
    }

    void invalidRulesHaveNoSideEffects_data()
    {
        QTest::addColumn<int>("mode");
        QTest::addColumn<QStringList>("sites");
        QTest::newRow("hostname") << 1 << QStringList { "example.com" };
        QTest::newRow("prefix") << 1 << QStringList { "198.51.100.0/99" };
        QTest::newRow("mode") << 3 << QStringList { "198.51.100.1" };
    }

    void invalidRulesHaveNoSideEffects()
    {
        QFETCH(int, mode);
        QFETCH(QStringList, sites);
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        const auto routes = daemon.backend.routes;
        QCOMPARE(daemon.updateSplitTunneling(mode, sites, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.routes, routes);
        QCOMPARE(daemon.backend.peerUpdates, 0);
    }

    void stalePeerIsRejected()
    {
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        QCOMPARE(daemon.updateSplitTunneling(1, { "198.51.100.1" }, "different-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.peerUpdates, 0);
    }

    void oldSessionWithTheSamePeerIsRejected()
    {
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        QCOMPARE(daemon.updateSplitTunneling(1, { "198.51.100.1" }, "test-peer", "previous-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.peerUpdates, 0);
    }

    void peerFailureRollsBackNewRoutes()
    {
        TestSplitTunnelDaemon daemon;
        const auto initial = initialConfiguration();
        daemon.seed(initial);
        const auto routes = daemon.backend.routes;
        daemon.backend.failNextPeerUpdate = true;
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.routes, routes);
        QCOMPARE(daemon.configuration().m_allowedIPAddressRanges, initial.m_allowedIPAddressRanges);
        QCOMPARE(daemon.backend.peer.m_allowedIPAddressRanges, initial.m_allowedIPAddressRanges);
        QCOMPARE(daemon.backend.initialPeerUpdates, 0);
        QCOMPARE(daemon.backend.livePeerUpdates, 2);
    }

    void routeFailureDoesNotCommitConfiguration()
    {
        TestSplitTunnelDaemon daemon;
        const auto initial = initialConfiguration();
        daemon.seed(initial);
        const auto routes = daemon.backend.routes;
        daemon.backend.failingRoute = "198.51.100.2/32";
        daemon.backend.partialRouteFailure = true;
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.1", "198.51.100.2" }, "test-peer",
                                             "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.routes, routes);
        QCOMPARE(daemon.configuration().m_allowedIPAddressRanges, initial.m_allowedIPAddressRanges);
        QCOMPARE(daemon.backend.livePeerUpdates, 0);
    }

    void defaultRestoreDoesNotClaimUnverifiedFirewallSuccess()
    {
        TestWireguardUtils backend;
        auto config = initialConfiguration();
        QCOMPARE(backend.WireguardUtils::restoreSplitTunneling(config), SplitTunnelingUpdateResult::StateUnknown);
        config.m_killSwitchEnabled = false;
        QCOMPARE(backend.WireguardUtils::restoreSplitTunneling(config), SplitTunnelingUpdateResult::Applied);
    }

    void confirmedPeerRejectionAllowsTheNextRequest()
    {
        TestSplitTunnelDaemon daemon;
        const auto initial = initialConfiguration();
        daemon.seed(initial);
        const auto routes = daemon.backend.routes;
        daemon.backend.rejectNextPeerUpdate = true;
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.routes, routes);
        QCOMPARE(daemon.backend.livePeerUpdates, 1);
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::Applied);
    }

    void failedPeerRollbackBlocksTheSession()
    {
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        daemon.backend.failAllPeerUpdates = true;
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::StateUnknown);
        const auto attempts = daemon.backend.livePeerUpdates;
        daemon.backend.failAllPeerUpdates = false;
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.2" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::StateUnknown);
        QCOMPARE(daemon.backend.livePeerUpdates, attempts);
    }

    void failedRouteRollbackIsNotAConfirmedRejection()
    {
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        daemon.backend.rejectNextPeerUpdate = true;
        daemon.backend.failingRemoval = "198.51.100.1/32";
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::StateUnknown);
    }

    void partialExclusionAddIsCompensatedBeforeReportingRejection()
    {
        TestSplitTunnelDaemon daemon;
        auto config = initialConfiguration();
        config.m_allowedIPAddressRanges = { IPAddress("0.0.0.0/0"), IPAddress("::/0") };
        daemon.seed(config);
        const auto exclusions = daemon.backend.exclusions;
        daemon.backend.failExclusionAdd = true;
        QCOMPARE(daemon.updateSplitTunneling(2, { "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.exclusions, exclusions);
        QCOMPARE(daemon.backend.livePeerUpdates, 0);
    }

    void partialExclusionRemovalRestoresTheRouteAndReferenceCount()
    {
        TestSplitTunnelDaemon daemon;
        auto config = initialConfiguration();
        config.m_allowedIPAddressRanges = { IPAddress("0.0.0.0/0"), IPAddress("::/0") };
        config.m_excludedAddresses.append("198.51.100.1/32");
        daemon.seed(config);
        const auto exclusions = daemon.backend.exclusions;
        daemon.backend.failExclusionRemoval = true;
        QCOMPARE(daemon.updateSplitTunneling(2, { }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.exclusions, exclusions);
        daemon.backend.failExclusionRemoval = false;
        QCOMPARE(daemon.updateSplitTunneling(2, { }, "test-peer", "test-session"), SplitTunnelingUpdateResult::Applied);
        QVERIFY(!daemon.backend.exclusions.contains(IPAddress("198.51.100.1/32")));
    }

    void failedExclusionCompensationIsUnknown()
    {
        TestSplitTunnelDaemon daemon;
        auto config = initialConfiguration();
        config.m_allowedIPAddressRanges = { IPAddress("0.0.0.0/0"), IPAddress("::/0") };
        daemon.seed(config);
        daemon.backend.failExclusionAdd = true;
        daemon.backend.failExclusionRemoval = true;
        QCOMPARE(daemon.updateSplitTunneling(2, { "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::StateUnknown);
    }

    void removalFailureRestoresPeerAndRoutes()
    {
        TestSplitTunnelDaemon daemon;
        const auto initial = initialConfiguration();
        daemon.seed(initial);
        const auto routes = daemon.backend.routes;
        daemon.backend.failingRemoval = "192.0.2.1/32";
        QCOMPARE(daemon.updateSplitTunneling(1, { "198.51.100.1" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.routes, routes);
        QCOMPARE(daemon.backend.peer.m_allowedIPAddressRanges, initial.m_allowedIPAddressRanges);
    }
    void failedAdditionPreservesPreexistingRoute()
    {
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        daemon.backend.routes.insert(IPAddress("198.51.100.9/32"));
        daemon.backend.rejectNextPeerUpdate = true;
        const auto before = daemon.backend.routes;
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.9" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.routes, before);
        QCOMPARE(daemon.backend.livePeerUpdates, 0);
    }

    void defaultRouteCollisionOnlyRollsBackCreatedHalf()
    {
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        daemon.backend.routes.insert(IPAddress("128.0.0.0/1"));
        const auto before = daemon.backend.routes;
        daemon.backend.expandDefaultRoutes = true;
        QCOMPARE(daemon.updateSplitTunneling(0, { }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.routes, before);
    }

    void preexistingExclusionIsNeverDeletedOnRejectedAddition()
    {
        TestSplitTunnelDaemon daemon;
        auto config = initialConfiguration();
        config.m_allowedIPAddressRanges = { IPAddress("0.0.0.0/0"), IPAddress("::/0") };
        daemon.seed(config);
        daemon.backend.exclusions.insert(IPAddress("198.51.100.9/32"));
        daemon.backend.rejectNextPeerUpdate = true;
        const auto before = daemon.backend.exclusions;
        QCOMPARE(daemon.updateSplitTunneling(2, { "198.51.100.9" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(daemon.backend.exclusions, before);
    }

    void missingRouteAcknowledgmentBlocksFurtherUpdates()
    {
        TestSplitTunnelDaemon daemon;
        daemon.seed(initialConfiguration());
        daemon.backend.unknownRouteCreation = true;
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.9" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::StateUnknown);
        daemon.backend.unknownRouteCreation = false;
        QCOMPARE(daemon.updateSplitTunneling(1, { "192.0.2.1", "198.51.100.10" }, "test-peer", "test-session"),
                 SplitTunnelingUpdateResult::StateUnknown);
        QCOMPARE(daemon.backend.livePeerUpdates, 0);
    }
};

QTEST_GUILESS_MAIN(TestSplitTunnelingDaemon)
#include "testSplitTunnelingDaemon.moc"
