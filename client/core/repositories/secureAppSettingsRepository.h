#ifndef SECUREAPPSETTINGSREPOSITORY_H
#define SECUREAPPSETTINGSREPOSITORY_H

#include <memory>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QByteArray>

#include "core/repositories/appSettingsRepository.h"
#include "secure_qsettings.h"

using namespace amnezia;

class SecureAppSettingsRepository : public AppSettingsRepository
{
public:
    explicit SecureAppSettingsRepository(SecureQSettings* settings);

    QLocale getAppLanguage() const override;
    void setAppLanguage(QLocale locale) override;

    bool useAmneziaDns() const override;
    void setUseAmneziaDns(bool enabled) override;
    QStringList getAllowedDnsServers() const override;
    void setAllowedDnsServers(const QStringList &servers) override;
    QString primaryDns() const override;
    void setPrimaryDns(const QString &dns) override;
    QString secondaryDns() const override;
    void setSecondaryDns(const QString &dns) override;

    RouteMode routeMode() const override;
    void setRouteMode(RouteMode mode) override;
    bool addVpnSite(RouteMode mode, const QString &site, const QString &ip = "") override;
    void addVpnSites(RouteMode mode, const QMap<QString, QString> &sites) override;
    void removeVpnSite(RouteMode mode, const QString &site) override;
    void removeAllVpnSites(RouteMode mode) override;
    QVariantMap vpnSites(RouteMode mode) const override;
    bool isSitesSplitTunnelingEnabled() const override;
    void setSitesSplitTunnelingEnabled(bool enabled) override;

    AppsRouteMode appsRouteMode() const override;
    void setAppsRouteMode(AppsRouteMode mode) override;
    void setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps) override;
    QVector<InstalledAppInfo> vpnApps(AppsRouteMode mode) const override;
    bool isAppsSplitTunnelingEnabled() const override;
    void setAppsSplitTunnelingEnabled(bool enabled) override;

    QString getGatewayEndpoint(bool isTestPurchase = false) const override;
    void setGatewayEndpoint(const QString &endpoint) override;
    void resetGatewayEndpoint() override;
    void setDevGatewayEndpoint() override;
    bool isDevGatewayEnv(bool isTestPurchase = false) const override;
    void toggleDevGatewayEnv(bool enabled) override;
    
    bool isKillSwitchEnabled() const override;
    void setKillSwitchEnabled(bool enabled) override;
    bool isStrictKillSwitchEnabled() const override;
    void setStrictKillSwitchEnabled(bool enabled) override;
    
    bool isAutoConnect() const override;
    void setAutoConnect(bool enabled) override;
    bool isStartMinimized() const override;
    void setStartMinimized(bool enabled) override;
    bool isScreenshotsEnabled() const override;
    void setScreenshotsEnabled(bool enabled) override;
    bool isSaveLogs() const override;
    void setSaveLogs(bool enabled) override;
    QDateTime getLogEnableDate() const override;
    void setLogEnableDate(const QDateTime &date) override;
    
    QString getInstallationUuid(bool createIfNotExists) const override;
    bool isHomeAdLabelVisible() const override;
    void disableHomeAdLabel() override;
    bool isPremV1MigrationReminderActive() const override;
    void disablePremV1MigrationReminder() override;
    QByteArray backupAppConfig() const override;
    bool restoreAppConfig(const QByteArray &cfg) override;
    void clearSettings() override;

    QString nextAvailableServerName() const override;

private:
    void setVpnSites(RouteMode mode, const QVariantMap &sites);
    void setInstallationUuid(const QString &uuid);
    
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    SecureQSettings* m_settings;
    QString m_gatewayEndpoint;
};

#endif // SECUREAPPSETTINGSREPOSITORY_H

