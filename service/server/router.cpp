#include "router.h"

#ifdef Q_OS_WIN
#include "router_win.h"
#elif defined (Q_OS_MAC)
#include "router_mac.h"
#endif


bool Router::routeAdd(const QString &ip, const QString &gw)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeAdd(ip, gw);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeAdd(ip, gw);
#endif
}

int Router::routeAddList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeAddList(gw, ips);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeAddList(gw, ips);
#endif
}

bool Router::clearSavedRoutes()
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().clearSavedRoutes();
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().clearSavedRoutes();
#endif
}

bool Router::routeDelete(const QString &ip, const QString &gw)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeDelete(ip, gw);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeDelete(ip, gw);
#endif
}

void Router::flushDns()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().flushDns();
#elif defined (Q_OS_MAC)
    RouterMac::Instance().flushDns();
#endif
}

