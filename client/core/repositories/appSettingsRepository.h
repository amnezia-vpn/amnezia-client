#ifndef APPSETTINGSREPOSITORY_H
#define APPSETTINGSREPOSITORY_H

#include <QLocale>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMap>
#include <QVector>

#include "containers/containers_defs.h"
#include "core/utils/defs.h"

using namespace amnezia;

using namespace amnezia;

class AppSettingsRepository
{
public:
    virtual ~AppSettingsRepository() = default;

    // Language settings
    virtual QLocale getAppLanguage() const = 0;
    virtual void setAppLanguage(QLocale locale) = 0;

    // DNS settings
    virtual bool useAmneziaDns() const = 0;
    virtual void setUseAmneziaDns(bool enabled) = 0;
    virtual QStringList getAllowedDnsServers() const = 0;
    virtual void setAllowedDnsServers(const QStringList &servers) = 0;
    virtual QString primaryDns() const = 0;
    virtual void setPrimaryDns(const QString &dns) = 0;
    virtual QString secondaryDns() const = 0;
    virtual void setSecondaryDns(const QString &dns) = 0;

    // Sites split tunneling
    virtual RouteMode routeMode() const = 0;
    virtual void setRouteMode(RouteMode mode) = 0;
    virtual bool addVpnSite(RouteMode mode, const QString &site, const QString &ip = "") = 0;
    virtual void addVpnSites(RouteMode mode, const QMap<QString, QString> &sites) = 0;
    virtual void removeVpnSite(RouteMode mode, const QString &site) = 0;
    virtual void removeAllVpnSites(RouteMode mode) = 0;
    virtual QVariantMap vpnSites(RouteMode mode) const = 0;
    virtual bool isSitesSplitTunnelingEnabled() const = 0;
    virtual void setSitesSplitTunnelingEnabled(bool enabled) = 0;

    // Apps split tunneling
    virtual AppsRouteMode appsRouteMode() const = 0;
    virtual void setAppsRouteMode(AppsRouteMode mode) = 0;
    virtual void setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps) = 0;
    virtual QVector<InstalledAppInfo> vpnApps(AppsRouteMode mode) const = 0;
    virtual bool isAppsSplitTunnelingEnabled() const = 0;
    virtual void setAppsSplitTunnelingEnabled(bool enabled) = 0;

    // Gateway settings
    virtual QString getGatewayEndpoint(bool isTestPurchase = false) const = 0;
    virtual void setGatewayEndpoint(const QString &endpoint) = 0;
    virtual void resetGatewayEndpoint() = 0;
    virtual void setDevGatewayEndpoint() = 0;
    virtual bool isDevGatewayEnv(bool isTestPurchase = false) const = 0;
    virtual void toggleDevGatewayEnv(bool enabled) = 0;
    
    // Kill switch settings
    virtual bool isKillSwitchEnabled() const = 0;
    virtual void setKillSwitchEnabled(bool enabled) = 0;
    virtual bool isStrictKillSwitchEnabled() const = 0;
    virtual void setStrictKillSwitchEnabled(bool enabled) = 0;
    
    // Application behavior settings
    virtual bool isAutoConnect() const = 0;
    virtual void setAutoConnect(bool enabled) = 0;
    virtual bool isStartMinimized() const = 0;
    virtual void setStartMinimized(bool enabled) = 0;
    virtual bool isScreenshotsEnabled() const = 0;
    virtual void setScreenshotsEnabled(bool enabled) = 0;
    virtual bool isSaveLogs() const = 0;
    virtual void setSaveLogs(bool enabled) = 0;
    virtual QDateTime getLogEnableDate() const = 0;
    virtual void setLogEnableDate(const QDateTime &date) = 0;
    
    // Other settings
    virtual QString getInstallationUuid(bool createIfNotExists) const = 0;
    virtual bool isHomeAdLabelVisible() const = 0;
    virtual void disableHomeAdLabel() = 0;
    virtual bool isPremV1MigrationReminderActive() const = 0;
    virtual void disablePremV1MigrationReminder() = 0;
    virtual QByteArray backupAppConfig() const = 0;
    virtual bool restoreAppConfig(const QByteArray &cfg) = 0;
    virtual void clearSettings() = 0;

    // Server name utilities
    virtual QString nextAvailableServerName() const = 0;
};

#endif // APPSETTINGSREPOSITORY_H

