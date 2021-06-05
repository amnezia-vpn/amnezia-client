#include "router_win.h"
#include "../client/utils.h"

#include <string>
#include <tlhelp32.h>
#include <tchar.h>

#include <QProcess>

LONG (NTAPI * NtSuspendProcess)(HANDLE ProcessHandle) = NULL;
LONG (NTAPI * NtResumeProcess)(HANDLE ProcessHandle)  = NULL;

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

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


    if (!Utils::checkIPv4Format(gw)) {
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
        QString ip = Utils::ipAddressFromIpWithSubnet(ipWithMask);

        if (!Utils::checkIPv4Format(ip)) {
            qCritical().noquote() << "Critical, trying to add invalid route, ip: " << ip;
            continue;
        }

        QString mask = Utils::netMaskFromIpWithSubnet(ip);

        // address
        ipfrow.dwForwardDest = inet_addr(ip.toStdString().c_str());

        // mask
        in_addr maskAddr;
        inet_pton(AF_INET, mask.toStdString().c_str(), &maskAddr);
        ipfrow.dwForwardMask = maskAddr.S_un.S_addr;

        dwStatus = CreateIpForwardEntry(&ipfrow);
        if (dwStatus == NO_ERROR){
            m_ipForwardRows.insert(ip, ipfrow);
            success_count++;
        }
        else if (dwStatus == ERROR_OBJECT_ALREADY_EXISTS) {
            m_ipForwardRows.insert(ip, ipfrow);
            success_count++;
            qDebug() << "Router::routeAdd: warning, route already exist:" << ip << gw;
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

    int removed_count = 0;
    for (auto i = m_ipForwardRows.begin(); i != m_ipForwardRows.end(); ++i) {
        dwStatus = DeleteIpForwardEntry(&i.value());

        if (dwStatus != ERROR_SUCCESS) {
            qDebug() << "Router::clearSavedRoutes : Could not delete old row" << i.key();
        }
        else  removed_count++;
    }

    if (pIpForwardTable)
        free(pIpForwardTable);

    qDebug() << "Router::clearSavedRoutes : removed routes:" << removed_count << "of" << m_ipForwardRows.size();
    m_ipForwardRows.clear();

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
    ULONG gw_addr= inet_addr(gw.toStdString().c_str());


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
        QString ip = Utils::ipAddressFromIpWithSubnet(ipMask);
        QString mask = Utils::netMaskFromIpWithSubnet(ipMask);

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

void RouterWin::flushDns()
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QString command = QString("ipconfig /flushdns");

    p.start(command);
    p.waitForFinished();
    //qDebug().noquote() << "OUTPUT ipconfig /flushdns: " + p.readAll();
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

