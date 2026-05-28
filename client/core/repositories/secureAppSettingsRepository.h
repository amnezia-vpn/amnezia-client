#ifndef SECUREAPPSETTINGSREPOSITORY_H
#define SECUREAPPSETTINGSREPOSITORY_H

#include <QObject>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QByteArray>

#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "secureQSettings.h"

using namespace amnezia;

class SecureAppSettingsRepository : public QObject
{
    Q_OBJECT

public:
    explicit SecureAppSettingsRepository(SecureQSettings* settings, QObject *parent = nullptr);

    QLocale getAppLanguage() const;
    void setAppLanguage(QLocale locale);

    bool useAmneziaDns() const;
    void setUseAmneziaDns(bool enabled);
    QStringList getAllowedDnsServers() const;
    void setAllowedDnsServers(const QStringList &servers);
    QString primaryDns() const;
    void setPrimaryDns(const QString &dns);
    QString secondaryDns() const;
    void setSecondaryDns(const QString &dns);

    RouteMode routeMode() const;
    void setRouteMode(RouteMode mode);
    bool addVpnSite(RouteMode mode, const QString &site, const QString &ip = "");
    void addVpnSites(RouteMode mode, const QMap<QString, QString> &sites);
    void removeVpnSite(RouteMode mode, const QString &site);
    void removeAllVpnSites(RouteMode mode);
    QVariantMap vpnSites(RouteMode mode) const;
    bool isSitesSplitTunnelingEnabled() const;
    void setSitesSplitTunnelingEnabled(bool enabled);

    AppsRouteMode appsRouteMode() const;
    void setAppsRouteMode(AppsRouteMode mode);
    void setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps);
    QVector<InstalledAppInfo> vpnApps(AppsRouteMode mode) const;
    bool isAppsSplitTunnelingEnabled() const;
    void setAppsSplitTunnelingEnabled(bool enabled);

    QString getGatewayEndpoint(bool isTestPurchase = false) const;
    void setGatewayEndpoint(const QString &endpoint);
    void resetGatewayEndpoint();
    void setDevGatewayEndpoint();
    bool isDevGatewayEnv(bool isTestPurchase = false) const;
    void toggleDevGatewayEnv(bool enabled);
    
    bool isKillSwitchEnabled() const;
    void setKillSwitchEnabled(bool enabled);
    bool isStrictKillSwitchEnabled() const;
    void setStrictKillSwitchEnabled(bool enabled);
    
    bool isAutoConnect() const;
    void setAutoConnect(bool enabled);
    bool isStartMinimized() const;
    void setStartMinimized(bool enabled);
    bool isScreenshotsEnabled() const;
    void setScreenshotsEnabled(bool enabled);
    bool isNewsNotifications() const;
    void setNewsNotifications(bool enabled);
    bool isSaveLogs() const;
    void setSaveLogs(bool enabled);
    QDateTime getLogEnableDate() const;
    void setLogEnableDate(const QDateTime &date);
    
    QString getInstallationUuid(bool createIfNotExists) const;
    QStringList getReadNewsIds() const;
    void setReadNewsIds(const QStringList &ids);

    bool isHomeAdLabelVisible() const;
    void disableHomeAdLabel();
    bool isPremV1MigrationReminderActive() const;
    void disablePremV1MigrationReminder();
    QByteArray backupAppConfig() const;
    bool restoreAppConfig(const QByteArray &cfg);
    void clearSettings();

    QByteArray xraySavedConfigs() const;
    void setXraySavedConfigs(const QByteArray &data);

signals:
    void appLanguageChanged(QLocale locale);
    void allowedDnsServersChanged(const QStringList &servers);
    void sitesChanged(RouteMode mode);
    void appsChanged(AppsRouteMode mode);
    void routeModeChanged(RouteMode mode);
    void appsRouteModeChanged(AppsRouteMode mode);
    void sitesSplitTunnelingEnabledChanged(bool enabled);
    void appsSplitTunnelingEnabledChanged(bool enabled);
    void useAmneziaDnsChanged(bool enabled);
    void saveLogsChanged(bool enabled);
    void screenshotsEnabledChanged(bool enabled);
    void settingsCleared();

private:
    void setVpnSites(RouteMode mode, const QVariantMap &sites);
    void setInstallationUuid(const QString &uuid);
    
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    SecureQSettings* m_settings;
    QString m_gatewayEndpoint;
};

#endif // SECUREAPPSETTINGSREPOSITORY_H

