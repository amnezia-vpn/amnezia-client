#ifndef ROUTERWIN_H
#define ROUTERWIN_H

#include <QTimer>
#include <QString>
#include <QSettings>
#include <QHash>
#include <QDebug>
#include <QObject>


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
class RouterWin : public QObject
{
    Q_OBJECT
public:
    static RouterWin& Instance();

    bool routeAdd(const QString &ip, const QString &gw, QString mask = QString());
    int routeAddList(const QString &gw, const QStringList &ips);
    bool clearSavedRoutes();
    bool routeDelete(const QString &ip, const QString &gw);
    void flushDns();

public slots:

private:
    RouterWin() {}
    RouterWin(RouterWin const &) = delete;
    RouterWin& operator= (RouterWin const&) = delete;

#ifdef Q_OS_WIN
    QList<MIB_IPFORWARDROW> ipForwardRows;
#endif
};

#endif // ROUTERWIN_H
