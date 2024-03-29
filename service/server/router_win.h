#ifndef ROUTERWIN_H
#define ROUTERWIN_H

#include <QTimer>
#include <QString>
#include <QSettings>
#include <QHash>
#include <QDebug>
#include <QObject>

#include "../client/platforms/windows/daemon/dnsutilswindows.h"

#include <WinSock2.h>  //includes Windows.h
#include <WS2tcpip.h>


#include <iphlpapi.h>
#include <IcmpAPI.h>
#include <stdio.h>
#include <stdlib.h>


#include <stdint.h>
//typedef uint8_t u8_t ;

//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

/**
 * @brief The Router class - General class for handling ip routing
 */
class RouterWin : public QObject
{
    Q_OBJECT
public:
    static RouterWin& Instance();

    int routeAddList(const QString &gw, const QStringList &ips);
    bool clearSavedRoutes();
    int routeDeleteList(const QString &gw, const QStringList &ips);
    void flushDns();
    void resetIpStack();

    void StartRoutingIpv6();
    void StopRoutingIpv6();

    void suspendWcmSvc(bool suspend);
    bool updateResolvers(const QString& ifname, const QList<QHostAddress>& resolvers);

private:
    RouterWin(RouterWin const &) = delete;
    RouterWin& operator= (RouterWin const&) = delete;

    DWORD GetServicePid(LPCWSTR serviceName);
    BOOL ListProcessThreads(DWORD dwOwnerPID);
    BOOL EnableDebugPrivilege();
    BOOL InitNtFunctions();
    BOOL SuspendProcess(BOOL fSuspend, DWORD dwProcessId);

private:
    RouterWin() {m_dnsUtil = new DnsUtilsWindows(this);}
    QMultiMap<QString, MIB_IPFORWARDROW> m_ipForwardRows;
    bool m_suspended = false;
    DnsUtilsWindows *m_dnsUtil; 
};

#endif // ROUTERWIN_H
