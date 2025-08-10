#ifndef ROUTERLINUX_H
#define ROUTERLINUX_H

#include <QTimer>
#include <QString>
#include <QSettings>
#include <QHash>
#include <QDebug>
#include <QObject>

#include "../client/platforms/linux/daemon/dnsutilslinux.h"

/**
 * @brief The Router class - General class for handling ip routing
 */
class RouterLinux : public QObject
{
    Q_OBJECT
public:
    struct Route {
        QString dst;
        QString gw;
    };

    static RouterLinux& Instance();

    bool routeAdd(const QString &ip, const QString &gw, const int &sock);
    int routeAddList(const QString &gw, const QStringList &ips);
    bool clearSavedRoutes();
    bool routeDelete(const QString &ip, const QString &gw, const int &sock);
    bool routeDeleteList(const QString &gw, const QStringList &ips);
    QString getgatewayandiface();
    void flushDns();
    bool createTun(const QString &dev, const QString &subnet);
    bool deleteTun(const QString &dev);
    void StartRoutingIpv6();
    void StopRoutingIpv6();
    bool updateResolvers(const QString& ifname, const QList<QHostAddress>& resolvers);
public slots:

private:
    RouterLinux() {m_dnsUtil = new DnsUtilsLinux(this);}
    RouterLinux(RouterLinux const &) = delete;
    RouterLinux& operator= (RouterLinux const&) = delete;

    bool isServiceActive(const QString &serviceName);
    QList<Route> m_addedRoutes;
    DnsUtilsLinux *m_dnsUtil;
};

#endif // ROUTERLINUX_H

