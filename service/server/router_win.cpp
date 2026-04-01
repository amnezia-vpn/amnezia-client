#include "router_win.h"

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

bool resolveTunInterfaceLuid(const QString &requestedName, NET_LUID &outLuid, QString &resolvedName)
{
    const QString requested = requestedName.trimmed();
    if (requested.isEmpty()) {
        return false;
    }

    DWORD res = ConvertInterfaceAliasToLuid(reinterpret_cast<const wchar_t*>(requested.utf16()), &outLuid);
    if (res == NO_ERROR) {
        resolvedName = requested;
        return true;
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

        // Clean up stale routes left from previous broken sessions before
        // attempting to create the fresh entry for the current session.
        for (DWORD rowIndex = 0; rowIndex < pIpForwardTable->dwNumEntries; ++rowIndex) {
            const MIB_IPFORWARDROW &existingRow = pIpForwardTable->table[rowIndex];
            if (!routeRowsEquivalent(existingRow, ipfrow)) {
                continue;
            }

            const DWORD deleteStatus = DeleteIpForwardEntry(const_cast<MIB_IPFORWARDROW *>(&existingRow));
            if (deleteStatus == NO_ERROR) {
                qDebug() << "Router::routeAddList: removed stale existing route before recreate:" << ipWithMask << gw;
            }
            break;
        }

        dwStatus = CreateIpForwardEntry(&ipfrow);
        if (dwStatus == NO_ERROR){
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

                m_ipForwardRows.insert(ipWithMask, existingRow);
                storedExistingRow = true;
                success_count++;
                break;
            }

            if (!storedExistingRow) {
                m_ipForwardRows.insert(ipWithMask, ipfrow);
                success_count++;
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
            if (!routeRowsEquivalent(currentRow, savedRow)) {
                continue;
            }

            return DeleteIpForwardEntry(&currentRow) == ERROR_SUCCESS;
        }

        return false;
    };

    int removed_count = 0;
    QMultiMap<QString, MIB_IPFORWARDROW> remainingRows;
    for (auto i = m_ipForwardRows.begin(); i != m_ipForwardRows.end(); ++i) {
        bool removed = DeleteIpForwardEntry(&i.value()) == ERROR_SUCCESS;
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
        if (!hasExpectedIp) {
            qWarning() << "RouterWin::createTun: address already exists but expected address not found on interface"
                       << dev << subnet;
        }
        return hasExpectedIp;
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
        QFuture<bool> res = QtConcurrent::mappedReduced(kIpv6Subnets, [loopback](const QString &subnet) -> bool {
            int res = QProcess::execute("netsh", { "interface", "ipv6", "add", "route", subnet, QString("interface=%1").arg(loopback.index()), "metric=0", "store=active" });
            return res == 0;
        },
        [](bool &result, bool success) {
            result = result && success;
        }, true);

        res.waitForFinished();
        return res.result();
    }

    return false;
}

bool RouterWin::StartRoutingIpv6()
{
    qDebug() << "RouterWin::StartRoutingIpv6";

    if (auto loopback = findLoopbackIface(); loopback.isValid()) {
        QFuture<bool> res = QtConcurrent::mappedReduced(kIpv6Subnets, [loopback](const QString &subnet) -> bool {
            int res = QProcess::execute("netsh", { "interface", "ipv6", "delete", "route", subnet, QString("interface=%1").arg(loopback.index()) });
            return res == 0;
        },
        [](bool &result, bool success) {
            result = result && success;
        }, true);

        res.waitForFinished();
        return res.result();
    }

    return false;
}
