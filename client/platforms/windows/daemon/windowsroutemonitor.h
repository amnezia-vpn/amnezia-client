/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSROUTEMONITOR_H
#define WINDOWSROUTEMONITOR_H

#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2ipdef.h>

#include <QHash>
#include <QMap>
#include <QObject>
#include <functional>

#include "daemon/routemutation.h"
#include "ipaddress.h"

struct WindowsRouteOperations
{
    std::function<DWORD(const MIB_IPFORWARD_ROW2 *)> createRoute = CreateIpForwardEntry2;
    std::function<DWORD(const MIB_IPFORWARD_ROW2 *)> deleteRoute = DeleteIpForwardEntry2;
    std::function<DWORD(ADDRESS_FAMILY, PMIB_IPFORWARD_TABLE2 *)> getRoutes = GetIpForwardTable2;
    std::function<DWORD(ADDRESS_FAMILY, PMIB_IPINTERFACE_TABLE *)> getInterfaces = GetIpInterfaceTable;
    std::function<void(void *)> freeTable = FreeMibTable;
};

class WindowsRouteMonitor final : public QObject
{
    Q_OBJECT

public:
    WindowsRouteMonitor(quint64 luid, QObject *parent, WindowsRouteOperations operations = { });
    ~WindowsRouteMonitor();

    bool setDetaultRouteCapture(bool enable);
    bool routesSynchronized() const
    {
        return m_routesSynchronized && !m_stopping;
    }
    RouteCreationResult createExclusionRoute(const IPAddress &prefix);

    bool addExclusionRoute(const IPAddress &prefix);
    bool deleteExclusionRoute(const IPAddress &prefix);
    bool flushExclusionRoutes()
    {
        m_stopping = true;
        return flushRouteTable(m_exclusionRoutes) && flushRouteTable(m_clonedRoutes);
    }

    quint64 getLuid() const
    {
        return m_luid;
    }

public slots:
    void routeChanged();

private:
    bool isRouteExcluded(const IP_ADDRESS_PREFIX *dest) const;
    static bool routeContainsDest(const IP_ADDRESS_PREFIX *route, const IP_ADDRESS_PREFIX *dest);
    static QHostAddress prefixToAddress(const IP_ADDRESS_PREFIX *dest);

    bool flushRouteTable(QHash<IPAddress, MIB_IPFORWARD_ROW2 *> &table);
    SplitTunnelingUpdateResult updateExclusionRoute(MIB_IPFORWARD_ROW2 *data, void *table);
    bool updateInterfaceMetrics(int family);
    bool updateCapturedRoutes(int family);
    bool updateCapturedRoutes(int family, void *table);

    QHash<IPAddress, MIB_IPFORWARD_ROW2 *> m_exclusionRoutes;
    QMap<quint64, ULONG> m_interfaceMetricsIpv4;
    QMap<quint64, ULONG> m_interfaceMetricsIpv6;

    // Default route cloning
    bool m_defaultRouteCapture = false;
    bool m_routesSynchronized = true;
    bool m_stopping = false;
    QHash<IPAddress, MIB_IPFORWARD_ROW2 *> m_clonedRoutes;

    const WindowsRouteOperations m_operations;
    const quint64 m_luid = 0;
    HANDLE m_routeHandle = INVALID_HANDLE_VALUE;
};

#endif /* WINDOWSROUTEMONITOR_H */
