#include "secureAppSettingsRepository.h"

#include "core/utils/defs.h"
#include "settings.h"

using namespace amnezia;

SecureAppSettingsRepository::SecureAppSettingsRepository(std::shared_ptr<Settings> settings)
    : m_settings(settings)
{
}

QLocale SecureAppSettingsRepository::getAppLanguage() const
{
    return m_settings->getAppLanguage();
}

void SecureAppSettingsRepository::setAppLanguage(QLocale locale)
{
    m_settings->setAppLanguage(locale);
}

bool SecureAppSettingsRepository::useAmneziaDns() const
{
    return m_settings->useAmneziaDns();
}

void SecureAppSettingsRepository::setUseAmneziaDns(bool enabled)
{
    m_settings->setUseAmneziaDns(enabled);
}

QStringList SecureAppSettingsRepository::getAllowedDnsServers() const
{
    return m_settings->allowedDnsServers();
}

void SecureAppSettingsRepository::setAllowedDnsServers(const QStringList &servers)
{
    m_settings->setAllowedDnsServers(servers);
}

QString SecureAppSettingsRepository::primaryDns() const
{
    return m_settings->primaryDns();
}

void SecureAppSettingsRepository::setPrimaryDns(const QString &dns)
{
    m_settings->setPrimaryDns(dns);
}

QString SecureAppSettingsRepository::secondaryDns() const
{
    return m_settings->secondaryDns();
}

void SecureAppSettingsRepository::setSecondaryDns(const QString &dns)
{
    m_settings->setSecondaryDns(dns);
}

RouteMode SecureAppSettingsRepository::routeMode() const
{
    return m_settings->routeMode();
}

void SecureAppSettingsRepository::setRouteMode(RouteMode mode)
{
    m_settings->setRouteMode(mode);
}

bool SecureAppSettingsRepository::addVpnSite(RouteMode mode, const QString &site, const QString &ip)
{
    return m_settings->addVpnSite(mode, site, ip);
}

void SecureAppSettingsRepository::addVpnSites(RouteMode mode, const QMap<QString, QString> &sites)
{
    m_settings->addVpnSites(mode, sites);
}

void SecureAppSettingsRepository::removeVpnSite(RouteMode mode, const QString &site)
{
    m_settings->removeVpnSite(mode, site);
}

void SecureAppSettingsRepository::removeAllVpnSites(RouteMode mode)
{
    m_settings->removeAllVpnSites(mode);
}

QVariantMap SecureAppSettingsRepository::vpnSites(RouteMode mode) const
{
    return m_settings->vpnSites(mode);
}

bool SecureAppSettingsRepository::isSitesSplitTunnelingEnabled() const
{
    return m_settings->isSitesSplitTunnelingEnabled();
}

void SecureAppSettingsRepository::setSitesSplitTunnelingEnabled(bool enabled)
{
    m_settings->setSitesSplitTunnelingEnabled(enabled);
}

AppsRouteMode SecureAppSettingsRepository::appsRouteMode() const
{
    return m_settings->getAppsRouteMode();
}

void SecureAppSettingsRepository::setAppsRouteMode(AppsRouteMode mode)
{
    m_settings->setAppsRouteMode(mode);
}

void SecureAppSettingsRepository::setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps)
{
    m_settings->setVpnApps(mode, apps);
}

QVector<InstalledAppInfo> SecureAppSettingsRepository::vpnApps(AppsRouteMode mode) const
{
    return m_settings->getVpnApps(mode);
}

bool SecureAppSettingsRepository::isAppsSplitTunnelingEnabled() const
{
    return m_settings->isAppsSplitTunnelingEnabled();
}

void SecureAppSettingsRepository::setAppsSplitTunnelingEnabled(bool enabled)
{
    m_settings->setAppsSplitTunnelingEnabled(enabled);
}

QString SecureAppSettingsRepository::getGatewayEndpoint(bool isTestPurchase) const
{
    return m_settings->getGatewayEndpoint(isTestPurchase);
}

void SecureAppSettingsRepository::setGatewayEndpoint(const QString &endpoint)
{
    m_settings->setGatewayEndpoint(endpoint);
}

void SecureAppSettingsRepository::resetGatewayEndpoint()
{
    m_settings->resetGatewayEndpoint();
}

void SecureAppSettingsRepository::setDevGatewayEndpoint()
{
    m_settings->setDevGatewayEndpoint();
}

bool SecureAppSettingsRepository::isDevGatewayEnv(bool isTestPurchase) const
{
    return m_settings->isDevGatewayEnv(isTestPurchase);
}

void SecureAppSettingsRepository::toggleDevGatewayEnv(bool enabled)
{
    m_settings->toggleDevGatewayEnv(enabled);
}

bool SecureAppSettingsRepository::isKillSwitchEnabled() const
{
    return m_settings->isKillSwitchEnabled();
}

void SecureAppSettingsRepository::setKillSwitchEnabled(bool enabled)
{
    m_settings->setKillSwitchEnabled(enabled);
}

bool SecureAppSettingsRepository::isStrictKillSwitchEnabled() const
{
    return m_settings->isStrictKillSwitchEnabled();
}

void SecureAppSettingsRepository::setStrictKillSwitchEnabled(bool enabled)
{
    m_settings->setStrictKillSwitchEnabled(enabled);
}

bool SecureAppSettingsRepository::isAutoConnect() const
{
    return m_settings->isAutoConnect();
}

void SecureAppSettingsRepository::setAutoConnect(bool enabled)
{
    m_settings->setAutoConnect(enabled);
}

bool SecureAppSettingsRepository::isStartMinimized() const
{
    return m_settings->isStartMinimized();
}

void SecureAppSettingsRepository::setStartMinimized(bool enabled)
{
    m_settings->setStartMinimized(enabled);
}

bool SecureAppSettingsRepository::isScreenshotsEnabled() const
{
    return m_settings->isScreenshotsEnabled();
}

void SecureAppSettingsRepository::setScreenshotsEnabled(bool enabled)
{
    m_settings->setScreenshotsEnabled(enabled);
}

bool SecureAppSettingsRepository::isSaveLogs() const
{
    return m_settings->isSaveLogs();
}

void SecureAppSettingsRepository::setSaveLogs(bool enabled)
{
    m_settings->setSaveLogs(enabled);
}

QDateTime SecureAppSettingsRepository::getLogEnableDate() const
{
    return m_settings->getLogEnableDate();
}

void SecureAppSettingsRepository::setLogEnableDate(const QDateTime &date)
{
    m_settings->setLogEnableDate(date);
}

QString SecureAppSettingsRepository::getInstallationUuid(bool createIfNotExists) const
{
    return m_settings->getInstallationUuid(createIfNotExists);
}

bool SecureAppSettingsRepository::isHomeAdLabelVisible() const
{
    return m_settings->isHomeAdLabelVisible();
}

void SecureAppSettingsRepository::disableHomeAdLabel()
{
    m_settings->disableHomeAdLabel();
}

bool SecureAppSettingsRepository::isPremV1MigrationReminderActive() const
{
    return m_settings->isPremV1MigrationReminderActive();
}

void SecureAppSettingsRepository::disablePremV1MigrationReminder()
{
    m_settings->disablePremV1MigrationReminder();
}

QByteArray SecureAppSettingsRepository::backupAppConfig() const
{
    return m_settings->backupAppConfig();
}

bool SecureAppSettingsRepository::restoreAppConfig(const QByteArray &cfg)
{
    return m_settings->restoreAppConfig(cfg);
}

void SecureAppSettingsRepository::clearSettings()
{
    m_settings->clearSettings();
}

QString SecureAppSettingsRepository::nextAvailableServerName() const
{
    return m_settings->nextAvailableServerName();
}

