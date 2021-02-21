#include "router.h"

#ifdef Q_OS_WIN
#include "router_win.h"
#elif defined (Q_OS_MAC)
#include "router_mac.h"
#endif


bool Router::routeAdd(const QString &ip, const QString &gw, QString mask)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeAdd(ip, gw, mask);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeAdd(ip, gw, mask);
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

bool Router::routeDelete(const QString &ip)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeDelete(ip);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeDelete(ip);
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

