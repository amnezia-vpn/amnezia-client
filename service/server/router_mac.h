#ifndef ROUTERMAC_H
#define ROUTERMAC_H

#include <QTimer>
#include <QString>
#include <QSettings>
#include <QHash>
#include <QDebug>
#include <QObject>


/**
 * @brief The Router class - General class for handling ip routing
 */
class RouterMac : public QObject
{
    Q_OBJECT
public:    
    static RouterMac& Instance();

    bool routeAdd(const QString &ip, const QString &gw);
    int routeAddList(const QString &gw, const QStringList &ips);
    bool clearSavedRoutes();
    bool routeDelete(const QString &ip, const QString &gw);
    bool routeDeleteList(const QString &gw, const QStringList &ips);
    void flushDns();

public slots:

private:
    RouterMac() {}
    RouterMac(RouterMac const &) = delete;
    RouterMac& operator= (RouterMac const&) = delete;

    QList<QString> m_addedRoutes;
};

#endif // ROUTERMAC_H

