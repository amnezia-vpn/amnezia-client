#include "router_win.h"

#include <string>
#include <tlhelp32.h>
#include <tchar.h>

#include <QProcess>
#include <QHostAddress>
#include <QScopeGuard>
#include <QtConcurrent>

#include <core/utils/networkUtilities.h>

LONG (NTAPI * NtSuspendProcess)(HANDLE ProcessHandle) = NULL;
LONG (NTAPI * NtResumeProcess)(HANDLE ProcessHandle)  = NULL;

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

QList<QString> RouterWin::kIpv6Subnets = { "fc00::/7", "2000::/4", "3000::/4" };

namespace {
quint32 ipv4Mask(int prefixLength)
{
    return prefixLength == 0 ? 0 : (0xffffffffu << (32 - prefixLength));
}

bool routeOverlapsIpv4Range(quint32 address, int prefixLength, quint32 base, int rangePrefixLength)
{
    const quint32 routeStart = address & ipv4Mask(prefixLength);
    const quint32 routeEnd = routeStart | ~ipv4Mask(prefixLength);
    const quint32 rangeStart = base & ipv4Mask(rangePrefixLength);
    const quint32 rangeEnd = rangeStart | ~ipv4Mask(rangePrefixLength);
    return routeStart <= rangeEnd && rangeStart <= routeEnd;
}

bool isRouteAddCandidate(const QString &ipWithMask)
{
    constexpr int minPublicBypassPrefixLength = 16;
    constexpr int minLocalBypassPrefixLength = 24;
    if (!NetworkUtilities::checkIpSubnetFormat(ipWithMask)) {
        return false;
    }

    const QString ip = NetworkUtilities::ipAddressFromIpWithSubnet(ipWithMask);
    const QHostAddress address(ip);
    if (address.protocol() != QAbstractSocket::IPv4Protocol) {
        return false;
    }

    int prefixLength = 32;
    const QStringList parts = ipWithMask.split("/");
    if (parts.size() == 2) {
        bool ok = false;
        prefixLength = parts.at(1).toInt(&ok);
        if (!ok || prefixLength < 0 || prefixLength > 32) {
            return false;
        }
    }
    if (prefixLength == 0) {
        return false;
    }

    const quint32 rawAddress = address.toIPv4Address();
    const auto inRange = [rawAddress](quint32 base, int prefix) {
        const quint32 mask = ipv4Mask(prefix);
        return (rawAddress & mask) == (base & mask);
    };
    const auto routeOverlapsRange = [rawAddress, prefixLength](quint32 base, int prefix) {
        return routeOverlapsIpv4Range(rawAddress, prefixLength, base, prefix);
    };
    if (prefixLength < 32 && (rawAddress & ipv4Mask(prefixLength)) != rawAddress) {
        return false;
    }
    if (address.isNull() || address.isLoopback() || address.isBroadcast()
        || address.isLinkLocal() || address.isMulticast()) {
        return false;
    }
    const bool localOrServiceRoute = inRange(0x0a000000u, 8)
        || inRange(0x64400000u, 10)
        || inRange(0xac100000u, 12)
        || inRange(0xc0a80000u, 16);
    if (routeOverlapsRange(0x00000000u, 8) || routeOverlapsRange(0x7f000000u, 8)
        || routeOverlapsRange(0xc0000000u, 24)
        || routeOverlapsRange(0xc0000200u, 24) || routeOverlapsRange(0xc01f0000u, 24)
        || routeOverlapsRange(0xc01fc400u, 24) || routeOverlapsRange(0xc034c100u, 24)
        || routeOverlapsRange(0xc0586300u, 24) || routeOverlapsRange(0xc0af3000u, 24)
        || routeOverlapsRange(0xc6120000u, 15) || routeOverlapsRange(0xc6336400u, 24)
        || routeOverlapsRange(0xcb007100u, 24) || routeOverlapsRange(0xe0000000u, 4)
        || routeOverlapsRange(0xf0000000u, 4)) {
        return false;
    }
    const int minPrefixLength = localOrServiceRoute
        ? minLocalBypassPrefixLength
        : minPublicBypassPrefixLength;
    return prefixLength >= minPrefixLength;
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
    return routeAddList(gw, ips, true);
}

int RouterWin::routeAddTrustedList(const QString &gw, const QStringList &ips)
{
    return routeAddList(gw, ips, false);
}

int RouterWin::routeAddList(const QString &gw, const QStringList &ips, bool validateRoutes)
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
    MIB_IPFORWARDROW ipfrow = {};

    ipfrow.dwForwardPolicy = 0;
    ipfrow.dwForwardAge = INFINITE;

    ipfrow.dwForwardNextHop = inet_addr(gw.toStdString().c_str());
    ipfrow.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;	/* XXX - next hop != final dest */
    ipfrow.dwForwardProto = MIB_IPPROTO_NETMGMT;	/* XXX - MIB_PROTO_NETMGMT */

    // Set iface for route
    IPAddr dwGwAddr = inet_addr(gw.toStdString().c_str());
    if (GetBestInterface(dwGwAddr, &ipfrow.dwForwardIfIndex) != NO_ERROR) {
        qDebug() << "Router::routeAddList : GetBestInterface failed";
        if (pIpForwardTable)
            free(pIpForwardTable);
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

    QStringList routeCandidates;
    routeCandidates.reserve(ips.size());
    for (const QString &ipWithMask : ips) {
        if (validateRoutes && !isRouteAddCandidate(ipWithMask)) {
            qWarning().noquote() << "Router::routeAddList: skipping non-routable split route:" << ipWithMask;
            continue;
        }
        if (!validateRoutes && !NetworkUtilities::checkIpSubnetFormat(ipWithMask)) {
            qWarning().noquote() << "Router::routeAddList: skipping invalid trusted split route:" << ipWithMask;
            continue;
        }
        if (!routeCandidates.contains(ipWithMask)) {
            routeCandidates.append(ipWithMask);
        }
    }

    auto trackManagedRoute = [this](const QString &routeKey, const MIB_IPFORWARDROW &row) {
        for (auto tracked = m_ipForwardRows.find(routeKey);
             tracked != m_ipForwardRows.end() && tracked.key() == routeKey;) {
            if (tracked.value().dwForwardDest == row.dwForwardDest &&
                tracked.value().dwForwardMask == row.dwForwardMask &&
                tracked.value().dwForwardNextHop == row.dwForwardNextHop) {
                tracked = m_ipForwardRows.erase(tracked);
                continue;
            }
            ++tracked;
        }
        m_ipForwardRows.insert(routeKey, row);
    };

    auto resumeSmallRouteBatchGuard = qScopeGuard([this] {
        if (m_suspended && m_ipForwardRows.size() <= 500) {
            suspendWcmSvc(false);
        }
    });

    if (routeCandidates.size() > 500) {
        suspendWcmSvc(true);
    }

    for (int i = 0; i < routeCandidates.size(); ++i) {
        QString ipWithMask = routeCandidates.at(i);
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

        dwStatus = CreateIpForwardEntry(&ipfrow);
        if (dwStatus == NO_ERROR){
            trackManagedRoute(ipWithMask, ipfrow);
            success_count++;
        }
        else if (dwStatus == ERROR_OBJECT_ALREADY_EXISTS) {
            bool deletedExisting = false;
            for (int rowIndex = 0; rowIndex < (int)pIpForwardTable->dwNumEntries; ++rowIndex) {
                MIB_IPFORWARDROW existing = pIpForwardTable->table[rowIndex];
                if (existing.dwForwardDest == ipfrow.dwForwardDest &&
                    existing.dwForwardMask == ipfrow.dwForwardMask &&
                    existing.dwForwardNextHop == ipfrow.dwForwardNextHop) {
                    DWORD deleteStatus = DeleteIpForwardEntry(&existing);
                    deletedExisting = deleteStatus == ERROR_SUCCESS || deleteStatus == ERROR_NOT_FOUND;
                    if (!deletedExisting) {
                        qWarning() << "Router::routeAdd: failed to replace existing route:" << ip << gw << deleteStatus;
                    }
                    break;
                }
            }
            if (deletedExisting) {
                dwStatus = CreateIpForwardEntry(&ipfrow);
                if (dwStatus == NO_ERROR) {
                    trackManagedRoute(ipWithMask, ipfrow);
                    success_count++;
                } else {
                    qDebug() << "Router::routeAdd: failed CreateIpForwardEntry() after replace, Error:" << ip << gw << dwStatus;
                }
            } else {
                qDebug() << "Router::routeAdd: warning, unmanaged route already exists:" << ip << gw;
            }
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
    auto resumeGuard = qScopeGuard([this] {
        if (m_suspended) suspendWcmSvc(false);
    });
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

    int removed_count = 0;
    for (auto i = m_ipForwardRows.begin(); i != m_ipForwardRows.end();) {
        dwStatus = DeleteIpForwardEntry(&i.value());

        if (dwStatus != ERROR_SUCCESS && dwStatus != ERROR_NOT_FOUND) {
            qDebug() << "Router::clearSavedRoutes : Could not delete old row" << i.key();
            ++i;
        }
        else {
            removed_count++;
            i = m_ipForwardRows.erase(i);
        }
    }

    if (pIpForwardTable)
        free(pIpForwardTable);

    qDebug() << "Router::clearSavedRoutes : removed routes:" << removed_count;

    return m_ipForwardRows.isEmpty();
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

        in_addr maskAddr = {};
        if (inet_pton(AF_INET, mask.toStdString().c_str(), &maskAddr) != 1) {
            continue;
        }

        ipMap.insert(inet_addr(ip.toStdString().c_str()), qMakePair(ipMask, maskAddr.S_un.S_addr));
    }

    for (int i = 0; i < (int) pIpForwardTable->dwNumEntries; i++) {
        MIB_IPFORWARDROW ipfrow = pIpForwardTable->table[i];

        if(ipMap.contains(ipfrow.dwForwardDest) &&
                ipMap.value(ipfrow.dwForwardDest).second == ipfrow.dwForwardMask &&
                ipfrow.dwForwardNextHop == gw_addr) {
            dwStatus = DeleteIpForwardEntry(&pIpForwardTable->table[i]);
            if (dwStatus == ERROR_SUCCESS) {
                const QString routeKey = ipMap.value(ipfrow.dwForwardDest).first;
                for (auto tracked = m_ipForwardRows.find(routeKey);
                     tracked != m_ipForwardRows.end() && tracked.key() == routeKey;) {
                    if (tracked.value().dwForwardDest == ipfrow.dwForwardDest &&
                        tracked.value().dwForwardMask == ipfrow.dwForwardMask &&
                        tracked.value().dwForwardNextHop == ipfrow.dwForwardNextHop) {
                        tracked = m_ipForwardRows.erase(tracked);
                        break;
                    }
                    ++tracked;
                }
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
    DWORD res = ConvertInterfaceAliasToLuid(reinterpret_cast<const wchar_t*>(dev.utf16()), &luid);
    if (res != NO_ERROR) {
        qCritical() << "Failed to convert luid: " << res;
        return false;
    }

    HANDLE hEvent = CreateEvent(nullptr, true, false, nullptr);
    if (!hEvent) {
        qCritical() << "Failed to allocate event object";
        return false;
    }
    auto _guardEvent = qScopeGuard([hEvent](){ CloseHandle(hEvent); });

    struct {
        HANDLE hEvent;
        NET_LUID luid;
        const QString &subnet;
        bool found;
    } ctx = { .hEvent = hEvent, .luid = luid, .subnet = subnet, .found = false };

    auto cb = [](void *priv, MIB_UNICASTIPADDRESS_ROW *row, MIB_NOTIFICATION_TYPE NotificationType) {
        auto* c = reinterpret_cast<decltype(ctx)*>(priv);
        if (row != nullptr && row->InterfaceLuid.Value == c->luid.Value && row->Address.si_family == AF_INET) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(row->Address.Ipv4.sin_family, &row->Address.Ipv4.sin_addr, ip, INET_ADDRSTRLEN);
            if (c->subnet == ip) {
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

    inet_pton(AF_INET, subnet.toStdString().c_str(), &row.Address.Ipv4.sin_addr);

    row.OnLinkPrefixLength = 32;
    row.ValidLifetime = 0xffffffff;
    row.PreferredLifetime = 0xffffffff;
    row.DadState = IpDadStatePreferred;

    res = CreateUnicastIpAddressEntry(&row);
    if (res != NO_ERROR && res != ERROR_OBJECT_ALREADY_EXISTS) {
        qDebug() << "Failed to create IP address:" << res;
        return false;
    }

    res = WaitForSingleObject(hEvent, 10000);
    if (res == WAIT_TIMEOUT) {
        qCritical() << "Timeout of waiting for IP assignment for " << dev << " device";
        return false;
    }

    return ctx.found;
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
    }

    return false;
}
