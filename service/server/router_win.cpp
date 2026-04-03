#include "router_win.h"

#include <cstring>
#include <string>
#include <tlhelp32.h>
#include <tchar.h>
#include <winternl.h>

#include <QProcess>
#include <QThread>
#include <QtConcurrent>

#include <core/networkUtilities.h>

LONG (NTAPI * NtSuspendProcess)(HANDLE ProcessHandle) = NULL;
LONG (NTAPI * NtResumeProcess)(HANDLE ProcessHandle)  = NULL;

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

QList<QString> RouterWin::kIpv6Subnets = { "fc00::/7", "2000::/4", "3000::/4" };

namespace {
bool routeRowsEquivalent(const MIB_IPFORWARDROW &lhs, const MIB_IPFORWARDROW &rhs)
{
    return lhs.dwForwardDest == rhs.dwForwardDest
           && lhs.dwForwardMask == rhs.dwForwardMask
           && lhs.dwForwardNextHop == rhs.dwForwardNextHop;
}

bool routeRowsSamePrefix(const MIB_IPFORWARDROW &lhs, const MIB_IPFORWARDROW &rhs)
{
    return lhs.dwForwardDest == rhs.dwForwardDest
           && lhs.dwForwardMask == rhs.dwForwardMask;
}

bool isRouteDeleteSuccess(DWORD status)
{
    return status == ERROR_SUCCESS || status == ERROR_NOT_FOUND;
}

bool interfaceHasIpv4Address(const NET_LUID &luid, const IN_ADDR &expectedAddress)
{
    PMIB_UNICASTIPADDRESS_TABLE table = nullptr;
    const DWORD status = GetUnicastIpAddressTable(AF_INET, &table);
    if (status != NO_ERROR || !table) {
        return false;
    }

    const auto tableGuard = qScopeGuard([table]() {
        FreeMibTable(table);
    });

    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_UNICASTIPADDRESS_ROW &row = table->Table[i];
        if (row.InterfaceLuid.Value != luid.Value || row.Address.si_family != AF_INET) {
            continue;
        }

        if (row.Address.Ipv4.sin_addr.S_un.S_addr == expectedAddress.S_un.S_addr) {
            return true;
        }
    }

    return false;
}

bool isTunLikeInterfaceName(const QNetworkInterface &iface, const QStringList &nameCandidates)
{
    const QString ifaceName = iface.name();
    const QString humanName = iface.humanReadableName();
    for (const QString &candidate : nameCandidates) {
        if (candidate.isEmpty()) {
            continue;
        }
        if (ifaceName.contains(candidate, Qt::CaseInsensitive)
            || humanName.contains(candidate, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool removeConflictingTunIpv4Addresses(const NET_LUID &targetLuid,
                                       const IN_ADDR &expectedAddress,
                                       const QString &requestedName)
{
    PMIB_UNICASTIPADDRESS_TABLE table = nullptr;
    const DWORD status = GetUnicastIpAddressTable(AF_INET, &table);
    if (status != NO_ERROR || !table) {
        return false;
    }

    const auto tableGuard = qScopeGuard([table]() {
        FreeMibTable(table);
    });

    QStringList tunCandidates = {
        requestedName.trimmed(),
        requestedName.trimmed() + QStringLiteral(" "),
        QStringLiteral("tun"),
        QStringLiteral("sing-tun"),
        QStringLiteral("wintun")
    };

    bool removedAny = false;
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        MIB_UNICASTIPADDRESS_ROW row = table->Table[i];
        if (row.InterfaceLuid.Value == targetLuid.Value || row.Address.si_family != AF_INET) {
            continue;
        }

        if (row.Address.Ipv4.sin_addr.S_un.S_addr != expectedAddress.S_un.S_addr) {
            continue;
        }

        const QNetworkInterface iface = QNetworkInterface::interfaceFromIndex(static_cast<int>(row.InterfaceIndex));
        const bool tunLike = !iface.isValid() || isTunLikeInterfaceName(iface, tunCandidates);
        if (!tunLike) {
            continue;
        }

        const DWORD deleteStatus = DeleteUnicastIpAddressEntry(&row);
        if (deleteStatus == NO_ERROR || deleteStatus == ERROR_NOT_FOUND) {
            removedAny = true;
            qWarning() << "RouterWin::createTun: removed conflicting IPv4 assignment from interface index"
                       << row.InterfaceIndex << "for address"
                       << QHostAddress(expectedAddress.S_un.S_addr).toString();
        }
    }

    return removedAny;
}

bool resolveTunInterfaceLuid(const QString &requestedName, NET_LUID &outLuid, QString &resolvedName)
{
    const QString requested = requestedName.trimmed();
    if (requested.isEmpty()) {
        return false;
    }

    DWORD res = ConvertInterfaceAliasToLuid(reinterpret_cast<const wchar_t*>(requested.utf16()), &outLuid);
    if (res == NO_ERROR) {
        NET_IFINDEX ifindex = 0;
        if (ConvertInterfaceLuidToIndex(&outLuid, &ifindex) == NO_ERROR) {
            const auto exactIface = QNetworkInterface::interfaceFromIndex(static_cast<int>(ifindex));
            if (exactIface.isValid() && (exactIface.flags() & QNetworkInterface::IsUp)) {
                resolvedName = exactIface.name().isEmpty() ? requested : exactIface.name();
                return true;
            }
        }
    }

    const QStringList candidates = {
        requested,
        requested + " ",
        QStringLiteral("sing-tun"),
        QStringLiteral("wintun"),
    };

    int bestScore = -1;
    int bestIndex = -1;
    QString bestName;

    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        const QString ifaceName = iface.name();
        const QString readableName = iface.humanReadableName();

        int score = -1;
        for (const QString &candidate : candidates) {
            if (candidate.isEmpty()) {
                continue;
            }
            if (ifaceName.compare(candidate, Qt::CaseInsensitive) == 0
                || readableName.compare(candidate, Qt::CaseInsensitive) == 0) {
                score = qMax(score, 300);
            } else if (ifaceName.startsWith(candidate, Qt::CaseInsensitive)
                       || readableName.startsWith(candidate, Qt::CaseInsensitive)) {
                score = qMax(score, 200);
            } else if (ifaceName.contains(candidate, Qt::CaseInsensitive)
                       || readableName.contains(candidate, Qt::CaseInsensitive)) {
                score = qMax(score, 100);
            }
        }

        if (score < 0) {
            continue;
        }

        if (iface.flags() & QNetworkInterface::IsUp) {
            score += 1000;
        }
        if (iface.flags() & QNetworkInterface::IsRunning) {
            score += 500;
        }
        score += iface.index();
        if (score > bestScore) {
            bestScore = score;
            bestIndex = iface.index();
            bestName = ifaceName.isEmpty() ? readableName : ifaceName;
        }
    }

    if (bestIndex <= 0) {
        return false;
    }

    res = ConvertInterfaceIndexToLuid(static_cast<NET_IFINDEX>(bestIndex), &outLuid);
    if (res != NO_ERROR) {
        return false;
    }

    resolvedName = bestName;
    return true;
}

bool ipv6RouteExistsOnInterface(ULONG interfaceIndex, const QString &subnet)
{
    if (interfaceIndex == 0) {
        return false;
    }

    const auto parsed = QHostAddress::parseSubnet(subnet);
    const QHostAddress expectedPrefix = parsed.first;
    const int expectedPrefixLength = parsed.second;
    if (expectedPrefix.protocol() != QAbstractSocket::IPv6Protocol || expectedPrefixLength < 0) {
        return false;
    }

    PMIB_IPFORWARD_TABLE2 table = nullptr;
    const DWORD status = GetIpForwardTable2(AF_INET6, &table);
    if (status != NO_ERROR || !table) {
        return false;
    }

    const auto tableGuard = qScopeGuard([table]() {
        FreeMibTable(table);
    });

    const Q_IPV6ADDR expectedBytes = expectedPrefix.toIPv6Address();

    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_IPFORWARD_ROW2 &row = table->Table[i];
        if (row.InterfaceIndex != interfaceIndex
            || row.DestinationPrefix.Prefix.si_family != AF_INET6
            || static_cast<int>(row.DestinationPrefix.PrefixLength) != expectedPrefixLength) {
            continue;
        }

        const QHostAddress routePrefix(row.DestinationPrefix.Prefix.Ipv6.sin6_addr.s6_addr);
        const Q_IPV6ADDR routeBytes = routePrefix.toIPv6Address();
        if (std::memcmp(routeBytes.c, expectedBytes.c, sizeof(expectedBytes.c)) == 0) {
            return true;
        }
    }

    return false;
}

bool executeNetshIpv6RouteCommand(const QStringList &arguments)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(QStringLiteral("netsh"), arguments);

    if (!process.waitForFinished(10000) || process.exitStatus() != QProcess::NormalExit) {
        qWarning() << "RouterWin: netsh command did not finish successfully:" << arguments
                   << "output:" << QString::fromLocal8Bit(process.readAll()).trimmed();
        return false;
    }

    const bool ok = process.exitCode() == 0;
    if (!ok) {
        qWarning() << "RouterWin: netsh command failed with exit code" << process.exitCode()
                   << "args:" << arguments
                   << "output:" << QString::fromLocal8Bit(process.readAll()).trimmed();
    }

    return ok;
}
}

RouterWin &RouterWin::Instance()
{
    static RouterWin s;
    BOOL ok;
    ok = s.InitNtFunctions();
    if (!ok) qDebug() << "RouterWin::Instance failed to InitNtFunctions";

    ok = s.EnableDebugPrivilege();
    if (!ok) qDebug() << "RouterWin::Instance failed to EnableDebugPrivilege";

    return s;
}

int RouterWin::routeAddList(const QString &gw, const QStringList &ips)
{
//    qDebug().noquote() << QString("ROUTE ADD List: IPs size:%1, GW: %2")
//                          .arg(ips.size())
//                          .arg(gw);

//    qDebug().noquote() << QString("ROUTE ADD List: IPs:\n%1")
//                          .arg(ips.join("\n"));


    if (!NetworkUtilities::checkIPv4Format(gw)) {
        qCritical().noquote() << "Trying to add invalid route, gw: " << gw;
        return 0;
    }

    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;
    DWORD dwStatus = 0;


    // Find out how big our buffer needs to be.
    dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
        // Allocate the memory for the table
        if (!(pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc(dwSize))) {
            qDebug() << "Malloc failed. Out of memory.";
            return 0;
        }
        // Now get the table.
        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }

    if (dwStatus != ERROR_SUCCESS) {
        qDebug() << "getIpForwardTable failed.";
        if (pIpForwardTable)
            free(pIpForwardTable);
        return 0;
    }

    int success_count = 0;
    MIB_IPFORWARDROW ipfrow;

    ipfrow.dwForwardPolicy = 0;
    ipfrow.dwForwardAge = INFINITE;

    ipfrow.dwForwardNextHop = inet_addr(gw.toStdString().c_str());
    ipfrow.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;	/* XXX - next hop != final dest */
    ipfrow.dwForwardProto = MIB_IPPROTO_NETMGMT;	/* XXX - MIB_PROTO_NETMGMT */

    // Set iface for route
    IPAddr dwGwAddr = inet_addr(gw.toStdString().c_str());
    if (GetBestInterface(dwGwAddr, &ipfrow.dwForwardIfIndex) != NO_ERROR) {
        qDebug() << "Router::routeAddList : GetBestInterface failed";
        return false;
    }

    // Get TAP iface metric to set it for new routes
    MIB_IPINTERFACE_ROW tap_iface;
    InitializeIpInterfaceEntry(&tap_iface);
    tap_iface.InterfaceIndex = ipfrow.dwForwardIfIndex;
    tap_iface.Family = AF_INET;
    dwStatus  = GetIpInterfaceEntry(&tap_iface);
    if (dwStatus == NO_ERROR){
        ipfrow.dwForwardMetric1 = tap_iface.Metric + 1;
    }
    else {
        qDebug() << "Router::routeAddList: failed GetIpInterfaceEntry(), Error:" << dwStatus;
        ipfrow.dwForwardMetric1 = 256;
    }
    ipfrow.dwForwardMetric2 = 0;
    ipfrow.dwForwardMetric3 = 0;
    ipfrow.dwForwardMetric4 = 0;
    ipfrow.dwForwardMetric5 = 0;

    for (int i = 0; i < ips.size(); ++i) {
        QString ipWithMask = ips.at(i);
        QString ip = NetworkUtilities::ipAddressFromIpWithSubnet(ipWithMask);

        if (!NetworkUtilities::checkIPv4Format(ip)) {
            qCritical().noquote() << "Critical, trying to add invalid route, ip: " << ip;
            continue;
        }

        QString mask = NetworkUtilities::netMaskFromIpWithSubnet(ipWithMask);

        // address
        ipfrow.dwForwardDest = inet_addr(ip.toStdString().c_str());

        // mask
        in_addr maskAddr;
        inet_pton(AF_INET, mask.toStdString().c_str(), &maskAddr);
        ipfrow.dwForwardMask = maskAddr.S_un.S_addr;

        // Clean up conflicting rows for the same destination prefix first.
        // Otherwise CreateIpForwardEntry may report "already exists" while the
        // route still points to an old gateway/interface from a previous session.
        for (DWORD rowIndex = 0; rowIndex < pIpForwardTable->dwNumEntries; ++rowIndex) {
            const MIB_IPFORWARDROW &existingRow = pIpForwardTable->table[rowIndex];
            if (!routeRowsSamePrefix(existingRow, ipfrow)) {
                continue;
            }

            if (existingRow.dwForwardType == MIB_IPROUTE_TYPE_DIRECT) {
                continue;
            }

            MIB_IPFORWARDROW rowToDelete = existingRow;
            const DWORD deleteStatus = DeleteIpForwardEntry(&rowToDelete);
            if (deleteStatus == NO_ERROR) {
                qDebug() << "Router::routeAddList: removed conflicting route before recreate:"
                         << ipWithMask << "oldGw=" << QHostAddress(existingRow.dwForwardNextHop).toString()
                         << "newGw=" << gw;
            }
        }

        dwStatus = CreateIpForwardEntry(&ipfrow);
        if (dwStatus == NO_ERROR){
            m_ipForwardRows.remove(ipWithMask);
            m_ipForwardRows.insert(ipWithMask, ipfrow);
            success_count++;
        }
        else if (dwStatus == ERROR_OBJECT_ALREADY_EXISTS) {
            bool storedExistingRow = false;
            for (DWORD rowIndex = 0; rowIndex < pIpForwardTable->dwNumEntries; ++rowIndex) {
                const MIB_IPFORWARDROW &existingRow = pIpForwardTable->table[rowIndex];
                if (!routeRowsEquivalent(existingRow, ipfrow)) {
                    continue;
                }

                m_ipForwardRows.remove(ipWithMask);
                m_ipForwardRows.insert(ipWithMask, existingRow);
                storedExistingRow = true;
                success_count++;
                break;
            }

            if (!storedExistingRow) {
                qWarning() << "Router::routeAdd: route already exists with different gateway/interface:"
                           << ipWithMask << "requestedGw=" << gw;
            }

            qDebug() << "Router::routeAdd: warning, route already exist:" << ipWithMask << gw;
        }
        else {
            qDebug() << "Router::routeAdd: failed CreateIpForwardEntry(), Error:" << ip << gw << dwStatus;
        }
    }

    // Free resources
    if (pIpForwardTable)
        free(pIpForwardTable);

    qDebug() << "Router::routeAddList finished, success: " << success_count << "/" << ips.size();

    if (m_ipForwardRows.size() > 500) suspendWcmSvc(true);

    return success_count;
}

bool RouterWin::clearSavedRoutes()
{
    if (m_ipForwardRows.isEmpty()) return true;

    qDebug() << "RouterWin::clearSavedRoutes forward rows size:" << m_ipForwardRows.size();

    // Declare and initialize variables
    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;
    DWORD dwStatus = 0;

    // Find out how big our buffer needs to be.
    dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
        // Allocate the memory for the table
        if (!(pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc(dwSize))) {
            qDebug() << "Router::clearSavedRoutes : Malloc failed. Out of memory";
            return false;
        }
        // Now get the table.
        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }

    if (dwStatus != ERROR_SUCCESS) {
        qDebug() << "Router::clearSavedRoutes : getIpForwardTable failed";
        if (pIpForwardTable)
            free(pIpForwardTable);

        return false;
    }

    auto deleteMatchingCurrentRow = [pIpForwardTable](const MIB_IPFORWARDROW &savedRow) -> bool {
        for (DWORD rowIndex = 0; rowIndex < pIpForwardTable->dwNumEntries; ++rowIndex) {
            MIB_IPFORWARDROW currentRow = pIpForwardTable->table[rowIndex];
            if (!routeRowsSamePrefix(currentRow, savedRow)) {
                continue;
            }

            if (currentRow.dwForwardType == MIB_IPROUTE_TYPE_DIRECT) {
                continue;
            }

            return isRouteDeleteSuccess(DeleteIpForwardEntry(&currentRow));
        }

        return false;
    };

    int removed_count = 0;
    QMultiMap<QString, MIB_IPFORWARDROW> remainingRows;
    for (auto i = m_ipForwardRows.begin(); i != m_ipForwardRows.end(); ++i) {
        const DWORD directDeleteStatus = DeleteIpForwardEntry(&i.value());
        bool removed = isRouteDeleteSuccess(directDeleteStatus);
        if (!removed) {
            removed = deleteMatchingCurrentRow(i.value());
        }

        if (!removed) {
            qDebug() << "Router::clearSavedRoutes : Could not delete old row" << i.key();
            remainingRows.insert(i.key(), i.value());
        } else {
            removed_count++;
        }
    }

    if (pIpForwardTable)
        free(pIpForwardTable);

    qDebug() << "Router::clearSavedRoutes : removed routes:" << removed_count << "of" << m_ipForwardRows.size();
    if (!remainingRows.isEmpty()) {
        qWarning() << "Router::clearSavedRoutes : keeping" << remainingRows.size()
                   << "routes for a later cleanup retry";
    }
    m_ipForwardRows = remainingRows;

    suspendWcmSvc(false);

    return true;
}

int RouterWin::routeDeleteList(const QString &gw, const QStringList &ips)
{
//    qDebug().noquote() << QString("ROUTE DELETE List: IPs size:%1, GW: %2")
//                          .arg(ips.size())
//                          .arg(gw);

//    qDebug().noquote() << QString("ROUTE DELETE List: IPs:\n%1")
//                          .arg(ips.join("\n"));

    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;
    DWORD dwStatus = 0;
    ULONG gw_addr = inet_addr(gw.toStdString().c_str());

    // Find out how big our buffer needs to be.
    dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
        // Allocate the memory for the table
        if (!(pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc(dwSize))) {
            qDebug() << "Malloc failed. Out of memory.";
            return 0;
        }
        // Now get the table.
        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }

    if (dwStatus != ERROR_SUCCESS) {
        qDebug() << "getIpForwardTable failed.";
        if (pIpForwardTable)
            free(pIpForwardTable);
        return 0;
    }

    int success_count = 0;

    QMap<ULONG, QPair<QString,ULONG>> ipMap;
    for (int i = 0; i < ips.size(); ++i) {
        QString ipMask = ips.at(i);
        if (ipMask.isEmpty()) continue;
        QString ip = NetworkUtilities::ipAddressFromIpWithSubnet(ipMask);
        QString mask = NetworkUtilities::netMaskFromIpWithSubnet(ipMask);

        if (ip.isEmpty()) continue;

        in_addr maskAddr;
        inet_pton(AF_INET, mask.toStdString().c_str(), &maskAddr);

        ipMap.insert(inet_addr(ip.toStdString().c_str()), qMakePair(ipMask, maskAddr.S_un.S_addr));
    }

    for (int i = 0; i < (int) pIpForwardTable->dwNumEntries; i++) {
        MIB_IPFORWARDROW ipfrow = pIpForwardTable->table[i];

        if(ipMap.contains(ipfrow.dwForwardDest) &&
                ipMap.value(ipfrow.dwForwardDest).second == ipfrow.dwForwardMask &&
                ipfrow.dwForwardNextHop == gw_addr) {
            dwStatus = DeleteIpForwardEntry(&pIpForwardTable->table[i]);
            if (dwStatus == ERROR_SUCCESS) {
                m_ipForwardRows.remove(ipMap.value(ipfrow.dwForwardDest).first);
                success_count++;
            }
        }
    }

    // Free resources
    if (pIpForwardTable)
        free(pIpForwardTable);

    qDebug() << "Router::routeDeleteList finished, success: " << success_count << "/" << ips.size();

    return success_count;
}

bool RouterWin::flushDns()
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QString command = QString("ipconfig /flushdns");

    p.start(command);
    p.waitForFinished();
    return true;
    //qDebug().noquote() << "OUTPUT ipconfig /flushdns: " + p.readAll();
}

void RouterWin::resetIpStack()
{
//    {
//        QProcess p;
//        QString command = QString("ipconfig /release");
//        p.start(command);
//    }
    {
        QProcess p;
        QString command = QString("netsh int ip reset");
        p.start(command);
        p.waitForFinished();
    }
    {
        QProcess p;
        QString command = QString("netsh winsock reset");
        p.start(command);
        p.waitForFinished();
    }
}

bool RouterWin::createTun(const QString &dev, const QString &subnet)
{
    NET_LUID luid;
    QString resolvedTunName;
    static constexpr int kResolveAttempts = 12;
    for (int attempt = 1; attempt <= kResolveAttempts; ++attempt) {
        if (resolveTunInterfaceLuid(dev, luid, resolvedTunName)) {
            break;
        }
        if (attempt < kResolveAttempts) {
            QThread::msleep(100);
        }
    }
    if (resolvedTunName.isEmpty()) {
        qCritical() << "RouterWin::createTun: failed to resolve tunnel interface for" << dev;
        return false;
    }
    qDebug() << "RouterWin::createTun: using interface" << resolvedTunName << "for requested name" << dev;

    DWORD res = NO_ERROR;

    HANDLE hEvent = CreateEvent(nullptr, true, false, nullptr);
    if (!hEvent) {
        qCritical() << "Failed to allocate event object";
        return false;
    }
    auto _guardEvent = qScopeGuard([hEvent](){ CloseHandle(hEvent); });

    IN_ADDR expectedAddress{};
    if (inet_pton(AF_INET, subnet.toStdString().c_str(), &expectedAddress) != 1) {
        qCritical() << "RouterWin::createTun: invalid subnet/IP" << subnet;
        return false;
    }

    // If the TUN already has the target address, consider createTun successful.
    if (interfaceHasIpv4Address(luid, expectedAddress)) {
        qDebug() << "RouterWin::createTun: TUN already has expected address" << subnet;
        return true;
    }

    struct {
        HANDLE hEvent;
        NET_LUID luid;
        ULONG expectedIpv4;
        bool found;
    } ctx = { .hEvent = hEvent, .luid = luid, .expectedIpv4 = expectedAddress.S_un.S_addr, .found = false };

    auto cb = [](void *priv, MIB_UNICASTIPADDRESS_ROW *row, MIB_NOTIFICATION_TYPE NotificationType) {
        Q_UNUSED(NotificationType);
        auto* c = reinterpret_cast<decltype(ctx)*>(priv);
        if (row != nullptr && row->InterfaceLuid.Value == c->luid.Value && row->Address.si_family == AF_INET) {
            if (row->Address.Ipv4.sin_addr.S_un.S_addr == c->expectedIpv4) {
                c->found = true;
                SetEvent(c->hEvent);
            }
        }
    };

    HANDLE hNotif;
    res = NotifyUnicastIpAddressChange(AF_INET, cb, &ctx, false, &hNotif);
    if (res != NO_ERROR) {
        qCritical() << "Failed to subscribe to interface change";
        return false;
    }
    auto _guardNotif = qScopeGuard([hNotif](){ CancelMibChangeNotify2(hNotif); });

    MIB_UNICASTIPADDRESS_ROW row;
    InitializeUnicastIpAddressEntry(&row);

    row.InterfaceLuid = luid;
    row.Address.si_family = AF_INET;

    row.Address.Ipv4.sin_addr = expectedAddress;

    row.OnLinkPrefixLength = 32;
    row.ValidLifetime = 0xffffffff;
    row.PreferredLifetime = 0xffffffff;
    row.DadState = IpDadStatePreferred;

    res = CreateUnicastIpAddressEntry(&row);
    if (res == ERROR_OBJECT_ALREADY_EXISTS) {
        const bool hasExpectedIp = interfaceHasIpv4Address(luid, expectedAddress);
        if (hasExpectedIp) {
            return true;
        }

        const bool removedConflicts = removeConflictingTunIpv4Addresses(luid, expectedAddress, dev);
        if (!removedConflicts) {
            qWarning() << "RouterWin::createTun: address already exists but expected address is not on the target interface"
                       << dev << subnet;
            return false;
        }

        QThread::msleep(120);
        res = CreateUnicastIpAddressEntry(&row);
        if (res == ERROR_OBJECT_ALREADY_EXISTS) {
            return interfaceHasIpv4Address(luid, expectedAddress);
        }
        if (res != NO_ERROR) {
            qCritical() << "RouterWin::createTun: failed to re-create IP address after conflict cleanup:" << res;
            return false;
        }
    }

    if (res != NO_ERROR) {
        qCritical() << "RouterWin::createTun: failed to create IP address:" << res;
        return false;
    }

    res = WaitForSingleObject(hEvent, 10000);
    if (res == WAIT_OBJECT_0 && ctx.found) {
        return true;
    }

    // Notification callbacks can be flaky on some Windows builds.
    // Trust the actual adapter state before failing the connection.
    if (interfaceHasIpv4Address(luid, expectedAddress)) {
        qWarning() << "RouterWin::createTun: notification was not received, but address is present";
        return true;
    }

    if (res == WAIT_TIMEOUT) {
        qCritical() << "Timeout of waiting for IP assignment for " << dev << " device";
    } else {
        qCritical() << "RouterWin::createTun: waiting for IP assignment failed:" << res;
    }

    if (!ctx.found) {
        qCritical() << "RouterWin::createTun: expected address was not observed on interface" << subnet;
        return false;
    }

    return true;
}

bool RouterWin::deleteTun(const QString &dev)
{
    QStringList candidateNames;
    if (!dev.isEmpty()) {
        candidateNames << dev;
    }
    candidateNames << "sing-tun";

    QList<DWORD> interfaceIndexes;
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        const QString interfaceName = iface.name();
        const QString readableName = iface.humanReadableName();

        bool matches = false;
        for (const QString &candidate : candidateNames) {
            if (interfaceName.contains(candidate, Qt::CaseInsensitive)
                || readableName.contains(candidate, Qt::CaseInsensitive)) {
                matches = true;
                break;
            }
        }

        if (matches && !interfaceIndexes.contains(static_cast<DWORD>(iface.index()))) {
            interfaceIndexes.append(static_cast<DWORD>(iface.index()));
        }
    }

    if (interfaceIndexes.isEmpty()) {
        qDebug() << "RouterWin::deleteTun: no matching tunnel interfaces found for" << dev;
        return true;
    }

    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;
    DWORD dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
        pIpForwardTable = reinterpret_cast<PMIB_IPFORWARDTABLE>(malloc(dwSize));
        if (!pIpForwardTable) {
            qWarning() << "RouterWin::deleteTun: malloc failed";
            return false;
        }
        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }

    if (dwStatus != ERROR_SUCCESS) {
        qWarning() << "RouterWin::deleteTun: GetIpForwardTable failed";
        if (pIpForwardTable) {
            free(pIpForwardTable);
        }
        return false;
    }

    int removedRoutes = 0;
    for (DWORD rowIndex = 0; rowIndex < pIpForwardTable->dwNumEntries; ++rowIndex) {
        MIB_IPFORWARDROW row = pIpForwardTable->table[rowIndex];
        if (!interfaceIndexes.contains(row.dwForwardIfIndex)) {
            continue;
        }

        if (row.dwForwardType == MIB_IPROUTE_TYPE_DIRECT) {
            continue;
        }

        if (DeleteIpForwardEntry(&row) == ERROR_SUCCESS) {
            removedRoutes++;
        }
    }

    if (pIpForwardTable) {
        free(pIpForwardTable);
    }

    qDebug() << "RouterWin::deleteTun: removed" << removedRoutes
             << "routes for tunnel interfaces" << interfaceIndexes;
    return true;
}

void RouterWin::suspendWcmSvc(bool suspend)
{
    if (suspend == m_suspended) return;

    // Solve Windows bug (routes > 1000)
    DWORD wcmSvcPid = GetServicePid(std::wstring(L"wcmSvc").c_str());

    //ListProcessThreads(wcmSvcPid);
    BOOL ok = SuspendProcess(suspend, wcmSvcPid);
    if (ok) {
        m_suspended = suspend;
    }

    qDebug() << "RouterWin::routeAddList" <<
                (ok ? "succeed to" : "failed to") <<
                (suspend ? "suspend wcmSvc" : "resume wcmSvc");

}

DWORD RouterWin::GetServicePid(LPCWSTR serviceName)
{
    const auto hScm = OpenSCManagerW(nullptr, nullptr, NULL);
    const auto hSc = OpenServiceW(hScm, serviceName, SERVICE_QUERY_STATUS);

    SERVICE_STATUS_PROCESS ssp = {};
    DWORD bytesNeeded = 0;
    QueryServiceStatusEx(hSc, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytesNeeded);

    CloseServiceHandle(hSc);
    CloseServiceHandle(hScm);

    return ssp.dwProcessId;
}

BOOL RouterWin::ListProcessThreads( DWORD dwOwnerPID )
{
  HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
  THREADENTRY32 te32;

  // Take a snapshot of all running threads
  hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
  if( hThreadSnap == INVALID_HANDLE_VALUE )
    return( FALSE );

  // Fill in the size of the structure before using it.
  te32.dwSize = sizeof(THREADENTRY32);

  // Retrieve information about the first thread,
  // and exit if unsuccessful
  if( !Thread32First( hThreadSnap, &te32 ) )
  {
    //printError( TEXT("Thread32First") ); // show cause of failure
    CloseHandle( hThreadSnap );          // clean the snapshot object
    return( FALSE );
  }

  // Now walk the thread list of the system,
  // and display information about each thread
  // associated with the specified process
  //HANDLE threadHandle;
  do
  {
    if( te32.th32OwnerProcessID == dwOwnerPID )
    {
        HANDLE threadHandle = OpenThread (PROCESS_QUERY_INFORMATION, FALSE, te32.th32ThreadID);
         qDebug() << "OpenThread GetLastError:"<< te32.th32ThreadID << GetLastError() << threadHandle;
        ULONG64 cycles = 0;
        BOOL ok = QueryThreadCycleTime(threadHandle, &cycles);
        qDebug() << "QueryThreadCycleTime GetLastError:" << ok << GetLastError();

        qDebug() << "Thread cycles:" << te32.th32ThreadID << cycles;
//      _tprintf( TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID );
//      _tprintf( TEXT("\n     Base priority  = %d"), te32.tpBasePri );
//      _tprintf( TEXT("\n     Delta priority = %d"), te32.tpDeltaPri );
//      _tprintf( TEXT("\n"));

        CloseHandle(threadHandle);
    }
  } while( Thread32Next(hThreadSnap, &te32 ) );

  CloseHandle( hThreadSnap );
  return( TRUE );
}

BOOL RouterWin::EnableDebugPrivilege(VOID)
{
  HANDLE           hToken = NULL;
  TOKEN_PRIVILEGES priv;

  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    return FALSE;

  if (!LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
    return FALSE;

  priv.PrivilegeCount           = 1;
  priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  return AdjustTokenPrivileges(hToken, FALSE, &priv, sizeof(priv), NULL, NULL);
}

BOOL RouterWin::InitNtFunctions(VOID)
{
  HMODULE hModule;

  hModule = GetModuleHandleW(L"ntdll.dll");
  if (hModule == NULL)
    return FALSE;

  //NtSuspendProcess = (decltype(NtSuspendProcess))GetProcAddress(hModule, "NtSuspendThread");
  NtSuspendProcess = (decltype(NtSuspendProcess))GetProcAddress(hModule, "NtSuspendProcess");
  if (NtSuspendProcess == NULL)
    return FALSE;

  //NtResumeProcess = (decltype(NtResumeProcess))GetProcAddress(hModule, "NtResumeThread");
  NtResumeProcess = (decltype(NtResumeProcess))GetProcAddress(hModule, "NtResumeProcess");
  if (NtResumeProcess == NULL)
    return FALSE;

  return TRUE;
}

BOOL RouterWin::SuspendProcess(BOOL fSuspend, DWORD dwProcessId)
{
    HANDLE pHandle = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, dwProcessId);
    if (pHandle == NULL) return false;

    bool ok = ((fSuspend ? NtSuspendProcess : NtResumeProcess)(pHandle) == STATUS_SUCCESS);
    CloseHandle(pHandle);

    return ok;
}

bool RouterWin::updateResolvers(const QString& ifname, const QList<QHostAddress>& resolvers)
{
    return m_dnsUtil->updateResolvers(ifname, resolvers);
}

bool RouterWin::restoreResolvers() {
    return m_dnsUtil->restoreResolvers();
}

QNetworkInterface RouterWin::findLoopbackIface()
{
    for (auto iface : QNetworkInterface::allInterfaces()) {
        if (iface.flags() & QNetworkInterface::IsLoopBack) {
            return iface;
        }
    }
    return {};
}

bool RouterWin::StopRoutingIpv6()
{
    qDebug() << "RouterWin::StopRoutingIpv6";

    if (auto loopback = findLoopbackIface(); loopback.isValid()) {
        const ULONG loopbackIndex = static_cast<ULONG>(loopback.index());
        bool ok = true;

        for (const QString &subnet : kIpv6Subnets) {
            if (ipv6RouteExistsOnInterface(loopbackIndex, subnet)) {
                continue;
            }

            const QStringList addArgs = {
                QStringLiteral("interface"), QStringLiteral("ipv6"),
                QStringLiteral("add"), QStringLiteral("route"), subnet,
                QStringLiteral("interface=%1").arg(loopbackIndex),
                QStringLiteral("metric=0"),
                QStringLiteral("store=active")
            };

            executeNetshIpv6RouteCommand(addArgs);

            if (!ipv6RouteExistsOnInterface(loopbackIndex, subnet)) {
                qWarning() << "RouterWin::StopRoutingIpv6: failed to add IPv6 block route" << subnet
                           << "on interface" << loopbackIndex;
                ok = false;
            }
        }

        return ok;
    }

    return false;
}

bool RouterWin::StartRoutingIpv6()
{
    qDebug() << "RouterWin::StartRoutingIpv6";

    if (auto loopback = findLoopbackIface(); loopback.isValid()) {
        const ULONG loopbackIndex = static_cast<ULONG>(loopback.index());
        bool ok = true;

        for (const QString &subnet : kIpv6Subnets) {
            if (!ipv6RouteExistsOnInterface(loopbackIndex, subnet)) {
                continue;
            }

            const QStringList deleteActiveArgs = {
                QStringLiteral("interface"), QStringLiteral("ipv6"),
                QStringLiteral("delete"), QStringLiteral("route"), subnet,
                QStringLiteral("interface=%1").arg(loopbackIndex),
                QStringLiteral("store=active")
            };
            executeNetshIpv6RouteCommand(deleteActiveArgs);

            // Some Windows builds reject `store=active` for delete route.
            // Retry with the minimal syntax before declaring failure.
            if (ipv6RouteExistsOnInterface(loopbackIndex, subnet)) {
                const QStringList deleteArgs = {
                    QStringLiteral("interface"), QStringLiteral("ipv6"),
                    QStringLiteral("delete"), QStringLiteral("route"), subnet,
                    QStringLiteral("interface=%1").arg(loopbackIndex)
                };
                executeNetshIpv6RouteCommand(deleteArgs);
            }

            if (ipv6RouteExistsOnInterface(loopbackIndex, subnet)) {
                qWarning() << "RouterWin::StartRoutingIpv6: failed to remove IPv6 block route" << subnet
                           << "from interface" << loopbackIndex;
                ok = false;
            }
        }

        return ok;
    }

    return false;
}
