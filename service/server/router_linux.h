#ifndef ROUTERLINUX_H
#define ROUTERLINUX_H

#include <QTimer>
#include <QString>
#include <QSettings>
#include <QHash>
#include <QDebug>
#include <QObject>

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

public slots:

private:
    RouterLinux() {}
    RouterLinux(RouterLinux const &) = delete;
    RouterLinux& operator= (RouterLinux const&) = delete;

    QList<Route> m_addedRoutes;
};

#endif // ROUTERLINUX_H

