#include "router.h"

#ifdef Q_OS_WIN
#include "router_win.h"
#elif defined (Q_OS_MAC)
#include "router_mac.h"
#elif defined Q_OS_LINUX
#include "router_linux.h"
#endif


int Router::routeAddList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeAddList(gw, ips);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeAddList(gw, ips);
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().routeAddList(gw, ips);
#endif
}

bool Router::clearSavedRoutes()
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().clearSavedRoutes();
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().clearSavedRoutes();
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().clearSavedRoutes();
#endif
}

int Router::routeDeleteList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeDeleteList(gw, ips);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeDeleteList(gw, ips);
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().routeDeleteList(gw, ips);
#endif
}

void Router::flushDns()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().flushDns();
#elif defined (Q_OS_MAC)
    RouterMac::Instance().flushDns();
#elif defined Q_OS_LINUX
    RouterLinux::Instance().flushDns();
#endif
}

void Router::resetIpStack()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().resetIpStack();
#elif defined (Q_OS_MAC)
    // todo fixme
#elif defined Q_OS_LINUX
    // todo fixme
#endif
}

bool Router::createTun(const QString &dev, const QString &subnet)
{
#ifdef Q_OS_LINUX
    return RouterLinux::Instance().createTun(dev, subnet);
#endif
#ifdef Q_OS_MAC
    return RouterMac::Instance().createTun(dev, subnet);
#endif
    return true;
};

bool Router::deleteTun(const QString &dev)
{
#ifdef Q_OS_LINUX
    return RouterLinux::Instance().deleteTun(dev);
#endif
#ifdef Q_OS_MAC
    return RouterMac::Instance().deleteTun(dev);
#endif
    return true;
};


void Router::StopRoutingIpv6()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().StopRoutingIpv6();
#elif defined (Q_OS_MAC)
    // todo fixme
#elif defined Q_OS_LINUX
    RouterLinux::Instance().StopRoutingIpv6();
#endif
}

void Router::StartRoutingIpv6()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().StartRoutingIpv6();
#elif defined (Q_OS_MAC)
    // todo fixme
#elif defined Q_OS_LINUX
    RouterLinux::Instance().StartRoutingIpv6();
#endif
}

