#ifndef ROUTER_H
#define ROUTER_H

#include <QTimer>
#include <QString>
#include <QSettings>
#include <QHash>
#include <QDebug>
#include <QObject>

/**
 * @brief The Router class - General class for handling ip routing
 */
class Router : public QObject
{
    Q_OBJECT
public:
    static int routeAddList(const QString &gw, const QStringList &ips);
    static bool clearSavedRoutes();
    static int routeDeleteList(const QString &gw, const QStringList &ips);
    static void flushDns();
};

#endif // ROUTER_H

