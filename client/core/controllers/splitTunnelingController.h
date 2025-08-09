#ifndef SPLITTUNNELINGCONTROLLER_H
#define SPLITTUNNELINGCONTROLLER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QStringList>
#include <QSharedPointer>
#include <QHostInfo>

#include "settings.h"
#include "core/defs.h"
#include "vpnconnection.h"

using namespace amnezia;

class SplitTunnelingController : public QObject
{
    Q_OBJECT

public:
    explicit SplitTunnelingController(std::shared_ptr<Settings> settings, 
                                      QSharedPointer<VpnConnection> vpnConnection = nullptr,
                                      QObject *parent = nullptr);

    // Apps split tunneling
    bool addApp(const InstalledAppInfo &appInfo);
    bool removeApp(const InstalledAppInfo &appInfo);
    QVector<InstalledAppInfo> getApps(Settings::AppsRouteMode routeMode) const;
    
    Settings::AppsRouteMode getAppsRouteMode() const;
    void setAppsRouteMode(Settings::AppsRouteMode routeMode);
    
    bool isAppsSplitTunnelingEnabled() const;
    void setAppsSplitTunnelingEnabled(bool enabled);

    // Sites split tunneling  
    bool addSite(const QString &hostname, const QString &ip = QString());
    bool addSites(const QMap<QString, QString> &sites, bool replaceExisting = false);
    bool removeSite(const QString &hostname);
    QVector<QPair<QString, QString>> getSites(Settings::RouteMode routeMode) const;
    
    Settings::RouteMode getSitesRouteMode() const;
    void setSitesRouteMode(Settings::RouteMode routeMode);
    
    bool isSitesSplitTunnelingEnabled() const;
    void setSitesSplitTunnelingEnabled(bool enabled);

signals:
    // Apps signals
    void appsRouteModelChanged();
    void appsSplitTunnelingToggled();
    void appAdded(const InstalledAppInfo &appInfo);
    void appRemoved(const InstalledAppInfo &appInfo);

    // Sites signals
    void sitesRouteModelChanged();
    void sitesSplitTunnelingToggled();
    void siteAdded(const QString &hostname, const QString &ip);
    void siteRemoved(const QString &hostname);

private slots:
    void handleHostnameResolved(const QHostInfo &hostInfo);

private:
    std::shared_ptr<Settings> m_settings;
    QSharedPointer<VpnConnection> m_vpnConnection;
    QMap<int, QString> m_pendingResolutions;
};

#endif // SPLITTUNNELINGCONTROLLER_H 
