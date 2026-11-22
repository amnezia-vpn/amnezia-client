#include <QQueue>
#include <QSet>
#include <QTest>
#include <cstdlib>

#include "platforms/windows/daemon/windowsroutemonitor.h"

class TestWindowsRouteMonitor : public QObject
{
    Q_OBJECT

    quint64 gatewayInterface = 3;
    QQueue<DWORD> creationErrors;
    QQueue<DWORD> deletionErrors;
    QSet<quint64> active;
    int deletions = 0;
    int creations = 0;
    int releases = 0;
    bool failRouteRead = false;
    bool failInterfaceRead = false;
    bool exactRoute = false;

    WindowsRouteOperations operations()
    {
        WindowsRouteOperations operations;
        operations.createRoute = [this](const auto *row) -> DWORD {
            ++creations;
            const auto error = creationErrors.isEmpty() ? NO_ERROR : creationErrors.dequeue();
            if (error == NO_ERROR)
                active.insert(row->InterfaceLuid.Value);
            return error;
        };
        operations.deleteRoute = [this](const auto *row) -> DWORD {
            ++deletions;
            const auto error = deletionErrors.isEmpty() ? NO_ERROR : deletionErrors.dequeue();
            if (error == NO_ERROR)
                active.remove(row->InterfaceLuid.Value);
            return error;
        };
        operations.getRoutes = [this](ADDRESS_FAMILY, PMIB_IPFORWARD_TABLE2 *result) -> DWORD {
            if (failRouteRead)
                return ERROR_ACCESS_DENIED;
            auto *table = static_cast<PMIB_IPFORWARD_TABLE2>(std::calloc(1, sizeof(MIB_IPFORWARD_TABLE2)));
            table->NumEntries = 1;
            table->Table[0].InterfaceLuid.Value = gatewayInterface;
            table->Table[0].DestinationPrefix.Prefix.si_family = AF_INET;
            table->Table[0].NextHop.si_family = AF_INET;
            if (exactRoute) {
                table->Table[0].DestinationPrefix.PrefixLength = 32;
                table->Table[0].DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = htonl(0xc0000201);
            }
            *result = table;
            return NO_ERROR;
        };
        operations.getInterfaces = [this](ADDRESS_FAMILY, PMIB_IPINTERFACE_TABLE *result) -> DWORD {
            if (failInterfaceRead)
                return ERROR_ACCESS_DENIED;
            auto *table = static_cast<PMIB_IPINTERFACE_TABLE>(std::calloc(1, sizeof(MIB_IPINTERFACE_TABLE)));
            table->NumEntries = 1;
            table->Table[0].Family = AF_INET;
            table->Table[0].InterfaceLuid.Value = gatewayInterface;
            table->Table[0].Connected = true;
            *result = table;
            return NO_ERROR;
        };
        operations.freeTable = [this](void *table) {
            ++releases;
            std::free(table);
        };
        return operations;
    }

private slots:
    void init()
    {
        gatewayInterface = 3;
        creationErrors.clear();
        deletionErrors.clear();
        active.clear();
        deletions = creations = releases = 0;
        failRouteRead = failInterfaceRead = exactRoute = false;
    }

    void failedDiscoveryDoesNotCreateARoute_data()
    {
        QTest::addColumn<bool>("interfaceFailure");
        QTest::newRow("route-table") << false;
        QTest::newRow("interface-table") << true;
    }

    void failedDiscoveryDoesNotCreateARoute()
    {
        QFETCH(bool, interfaceFailure);
        WindowsRouteMonitor monitor(99, nullptr, operations());
        failInterfaceRead = interfaceFailure;
        failRouteRead = !interfaceFailure;
        QCOMPARE(monitor.createExclusionRoute(IPAddress("192.0.2.1")).result, SplitTunnelingUpdateResult::NotApplied);
        QCOMPARE(creations, 0);
        QCOMPARE(releases, interfaceFailure ? 1 : 0);
    }

    void captureReadFailureDoesNotUseAnUninitializedTable()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        const IPAddress address("192.0.2.1");
        QCOMPARE(monitor.createExclusionRoute(address).result, SplitTunnelingUpdateResult::Applied);
        QVERIFY(monitor.setDetaultRouteCapture(true));
        const int priorReleases = releases;
        failRouteRead = true;
        QVERIFY(!monitor.deleteExclusionRoute(address));
        QVERIFY(!monitor.routesSynchronized());
        QCOMPARE(releases, priorReleases);
    }

    void failedCreationIsNotApplied()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        creationErrors.enqueue(ERROR_ACCESS_DENIED);
        QCOMPARE(monitor.createExclusionRoute(IPAddress("192.0.2.1")).result, SplitTunnelingUpdateResult::NotApplied);
        QVERIFY(active.isEmpty());
        QVERIFY(monitor.flushExclusionRoutes());
        QCOMPARE(deletions, 0);
    }

    void failedDeletionRemainsAvailableForRetry()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        const IPAddress address("192.0.2.1");
        QCOMPARE(monitor.createExclusionRoute(address).result, SplitTunnelingUpdateResult::Applied);
        deletionErrors.enqueue(ERROR_ACCESS_DENIED);
        QVERIFY(!monitor.deleteExclusionRoute(address));
        QVERIFY(active.contains(3));
        QVERIFY(monitor.deleteExclusionRoute(address));
        QCOMPARE(deletions, 2);
        QVERIFY(active.isEmpty());
    }

    void failedFlushRetainsItsLedger()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        QCOMPARE(monitor.createExclusionRoute(IPAddress("192.0.2.1")).result, SplitTunnelingUpdateResult::Applied);
        deletionErrors.enqueue(ERROR_ACCESS_DENIED);
        QVERIFY(!monitor.flushExclusionRoutes());
        gatewayInterface = 4;
        monitor.routeChanged();
        QCOMPARE(creations, 1);
        QCOMPARE(active, (QSet<quint64> { 3 }));
        QVERIFY(monitor.flushExclusionRoutes());
        QCOMPARE(deletions, 2);
    }

    void replacementFailureRestoresPreviousRoute()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        QCOMPARE(monitor.createExclusionRoute(IPAddress("192.0.2.1")).result, SplitTunnelingUpdateResult::Applied);
        gatewayInterface = 4;
        creationErrors.enqueue(ERROR_ACCESS_DENIED);
        creationErrors.enqueue(NO_ERROR);
        monitor.routeChanged();
        QCOMPARE(active, (QSet<quint64> { 3 }));
        QVERIFY(monitor.routesSynchronized());
    }

    void queuedRouteNotificationCannotRecreateRoutesAfterCleanup()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        const IPAddress address("192.0.2.1");
        QCOMPARE(monitor.createExclusionRoute(address).result, SplitTunnelingUpdateResult::Applied);
        QVERIFY(monitor.flushExclusionRoutes());
        gatewayInterface = 4;
        monitor.routeChanged();
        QCOMPARE(creations, 1);
        QVERIFY(active.isEmpty());
        QCOMPARE(monitor.createExclusionRoute(address).result, SplitTunnelingUpdateResult::StateUnknown);
    }

    void existingPhysicalRouteIsNeverDeleted()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        exactRoute = true;
        const IPAddress address("192.0.2.1");
        QCOMPARE(monitor.createExclusionRoute(address).result, SplitTunnelingUpdateResult::Applied);
        QCOMPARE(creations, 0);
        QVERIFY(monitor.deleteExclusionRoute(address));
        QCOMPARE(deletions, 0);
    }

    void replacementDeletionFailureKeepsPreviousRoute()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        QCOMPARE(monitor.createExclusionRoute(IPAddress("192.0.2.1")).result, SplitTunnelingUpdateResult::Applied);
        gatewayInterface = 4;
        deletionErrors.enqueue(ERROR_ACCESS_DENIED);
        monitor.routeChanged();
        QCOMPARE(active, (QSet<quint64> { 3 }));
        QVERIFY(monitor.routesSynchronized());
        QCOMPARE(creations, 1);
        QVERIFY(monitor.flushExclusionRoutes());
        QVERIFY(active.isEmpty());
    }

    void failedRestorationIsUnknown()
    {
        WindowsRouteMonitor monitor(99, nullptr, operations());
        QCOMPARE(monitor.createExclusionRoute(IPAddress("192.0.2.1")).result, SplitTunnelingUpdateResult::Applied);
        gatewayInterface = 4;
        creationErrors.enqueue(ERROR_ACCESS_DENIED);
        creationErrors.enqueue(ERROR_ACCESS_DENIED);
        monitor.routeChanged();
        QVERIFY(!monitor.routesSynchronized());
        QCOMPARE(monitor.createExclusionRoute(IPAddress("198.51.100.1")).result,
                 SplitTunnelingUpdateResult::StateUnknown);
    }
};

QTEST_GUILESS_MAIN(TestWindowsRouteMonitor)
#include "testWindowsRouteMonitor.moc"
