#ifndef QAPPSETTINGSREPOSITORY_H
#define QAPPSETTINGSREPOSITORY_H

#include <QObject>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMap>
#include <QVector>

#include "core/repositories/appSettingsRepository.h"
#include "settings.h"

using namespace amnezia;

class QAppSettingsRepository : public QObject, public AppSettingsRepository
{
    Q_OBJECT

public:
    explicit QAppSettingsRepository(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    // AppSettingsRepository interface
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

    QString getGatewayEndpoint() const override;
    void setGatewayEndpoint(const QString &endpoint) override;
    void resetGatewayEndpoint() override;
    void setDevGatewayEndpoint() override;
    bool isDevGatewayEnv() const override;
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
    std::shared_ptr<Settings> m_settings;
};

#endif // QAPPSETTINGSREPOSITORY_H

