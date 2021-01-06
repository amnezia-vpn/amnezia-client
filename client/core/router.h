#ifndef ROUTER_H
#define ROUTER_H

#include <QTimer>
#include <QString>
#include <QSettings>
#include <QHash>
#include <QDebug>


#ifdef Q_OS_WIN
#include <WinSock2.h>  //includes Windows.h
#include <WS2tcpip.h>


#include <iphlpapi.h>
#include <IcmpAPI.h>
#include <stdio.h>
#include <stdlib.h>


#include <stdint.h>
typedef uint8_t u8_t ;

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#endif //Q_OS_WIN


/**
 * @brief The Router class - General class for handling ip routing
 */
class Router : public QObject
{
    Q_OBJECT
public:
    static Router& Instance();

    bool routeAdd(const QString &ip, const QString &gw, QString mask = QString());
    int routeAddList(const QString &gw, const QStringList &ips);
    bool clearSavedRoutes();
    bool routeDelete(const QString &ip);
    void flushDns();

public slots:

private:
    Router() {}
    Router(Router const &) = delete;
    Router& operator= (Router const&) = delete;

#ifdef Q_OS_WIN
    QList<MIB_IPFORWARDROW> ipForwardRows;
#endif
};

#endif // ROUTER_H
