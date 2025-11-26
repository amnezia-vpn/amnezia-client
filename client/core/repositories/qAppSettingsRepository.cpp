#include "qAppSettingsRepository.h"

#include "core/defs.h"
#include "settings.h"

using namespace amnezia;

QAppSettingsRepository::QAppSettingsRepository(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

QLocale QAppSettingsRepository::getAppLanguage() const
{
    return m_settings->getAppLanguage();
}

void QAppSettingsRepository::setAppLanguage(QLocale locale)
{
    m_settings->setAppLanguage(locale);
    emit appLanguageChanged(locale);
}

bool QAppSettingsRepository::useAmneziaDns() const
{
    return m_settings->useAmneziaDns();
}

void QAppSettingsRepository::setUseAmneziaDns(bool enabled)
{
    m_settings->setUseAmneziaDns(enabled);
    emit useAmneziaDnsChanged(enabled);
}

QStringList QAppSettingsRepository::getAllowedDnsServers() const
{
    return m_settings->allowedDnsServers();
}

void QAppSettingsRepository::setAllowedDnsServers(const QStringList &servers)
{
    m_settings->setAllowedDnsServers(servers);
    emit allowedDnsServersChanged(servers);
}

QString QAppSettingsRepository::primaryDns() const
{
    return m_settings->primaryDns();
}

void QAppSettingsRepository::setPrimaryDns(const QString &dns)
{
    m_settings->setPrimaryDns(dns);
}

QString QAppSettingsRepository::secondaryDns() const
{
    return m_settings->secondaryDns();
}

void QAppSettingsRepository::setSecondaryDns(const QString &dns)
{
    m_settings->setSecondaryDns(dns);
}

RouteMode QAppSettingsRepository::routeMode() const
{
    return m_settings->routeMode();
}

void QAppSettingsRepository::setRouteMode(RouteMode mode)
{
    m_settings->setRouteMode(mode);
    emit routeModeChanged(mode);
}

bool QAppSettingsRepository::addVpnSite(RouteMode mode, const QString &site, const QString &ip)
{
    bool result = m_settings->addVpnSite(mode, site, ip);
    if (result) {
        emit sitesChanged(mode);
    }
    return result;
}

void QAppSettingsRepository::addVpnSites(RouteMode mode, const QMap<QString, QString> &sites)
{
    m_settings->addVpnSites(mode, sites);
    emit sitesChanged(mode);
}

void QAppSettingsRepository::removeVpnSite(RouteMode mode, const QString &site)
{
    m_settings->removeVpnSite(mode, site);
    emit sitesChanged(mode);
}

void QAppSettingsRepository::removeAllVpnSites(RouteMode mode)
{
    m_settings->removeAllVpnSites(mode);
    emit sitesChanged(mode);
}

QVariantMap QAppSettingsRepository::vpnSites(RouteMode mode) const
{
    return m_settings->vpnSites(mode);
}

bool QAppSettingsRepository::isSitesSplitTunnelingEnabled() const
{
    return m_settings->isSitesSplitTunnelingEnabled();
}

void QAppSettingsRepository::setSitesSplitTunnelingEnabled(bool enabled)
{
    m_settings->setSitesSplitTunnelingEnabled(enabled);
    emit sitesSplitTunnelingEnabledChanged(enabled);
}

AppsRouteMode QAppSettingsRepository::appsRouteMode() const
{
    return m_settings->getAppsRouteMode();
}

void QAppSettingsRepository::setAppsRouteMode(AppsRouteMode mode)
{
    m_settings->setAppsRouteMode(mode);
    emit appsRouteModeChanged(mode);
}

void QAppSettingsRepository::setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps)
{
    m_settings->setVpnApps(mode, apps);
    emit appsChanged(mode);
}

QVector<InstalledAppInfo> QAppSettingsRepository::vpnApps(AppsRouteMode mode) const
{
    return m_settings->getVpnApps(mode);
}

bool QAppSettingsRepository::isAppsSplitTunnelingEnabled() const
{
    return m_settings->isAppsSplitTunnelingEnabled();
}

void QAppSettingsRepository::setAppsSplitTunnelingEnabled(bool enabled)
{
    m_settings->setAppsSplitTunnelingEnabled(enabled);
    emit appsSplitTunnelingEnabledChanged(enabled);
}

QString QAppSettingsRepository::getGatewayEndpoint() const
{
    return m_settings->getGatewayEndpoint();
}

void QAppSettingsRepository::setGatewayEndpoint(const QString &endpoint)
{
    m_settings->setGatewayEndpoint(endpoint);
}

void QAppSettingsRepository::resetGatewayEndpoint()
{
    m_settings->resetGatewayEndpoint();
}

void QAppSettingsRepository::setDevGatewayEndpoint()
{
    m_settings->setDevGatewayEndpoint();
}

bool QAppSettingsRepository::isDevGatewayEnv() const
{
    return m_settings->isDevGatewayEnv();
}

void QAppSettingsRepository::toggleDevGatewayEnv(bool enabled)
{
    m_settings->toggleDevGatewayEnv(enabled);
}

bool QAppSettingsRepository::isKillSwitchEnabled() const
{
    return m_settings->isKillSwitchEnabled();
}

void QAppSettingsRepository::setKillSwitchEnabled(bool enabled)
{
    m_settings->setKillSwitchEnabled(enabled);
}

bool QAppSettingsRepository::isStrictKillSwitchEnabled() const
{
    return m_settings->isStrictKillSwitchEnabled();
}

void QAppSettingsRepository::setStrictKillSwitchEnabled(bool enabled)
{
    m_settings->setStrictKillSwitchEnabled(enabled);
}

bool QAppSettingsRepository::isAutoConnect() const
{
    return m_settings->isAutoConnect();
}

void QAppSettingsRepository::setAutoConnect(bool enabled)
{
    m_settings->setAutoConnect(enabled);
}

bool QAppSettingsRepository::isStartMinimized() const
{
    return m_settings->isStartMinimized();
}

void QAppSettingsRepository::setStartMinimized(bool enabled)
{
    m_settings->setStartMinimized(enabled);
}

bool QAppSettingsRepository::isScreenshotsEnabled() const
{
    return m_settings->isScreenshotsEnabled();
}

void QAppSettingsRepository::setScreenshotsEnabled(bool enabled)
{
    m_settings->setScreenshotsEnabled(enabled);
    emit screenshotsEnabledChanged(enabled);
}

bool QAppSettingsRepository::isSaveLogs() const
{
    return m_settings->isSaveLogs();
}

void QAppSettingsRepository::setSaveLogs(bool enabled)
{
    m_settings->setSaveLogs(enabled);
    emit saveLogsChanged(enabled);
}

QDateTime QAppSettingsRepository::getLogEnableDate() const
{
    return m_settings->getLogEnableDate();
}

void QAppSettingsRepository::setLogEnableDate(const QDateTime &date)
{
    m_settings->setLogEnableDate(date);
}

QString QAppSettingsRepository::getInstallationUuid(bool createIfNotExists) const
{
    return m_settings->getInstallationUuid(createIfNotExists);
}

bool QAppSettingsRepository::isHomeAdLabelVisible() const
{
    return m_settings->isHomeAdLabelVisible();
}

void QAppSettingsRepository::disableHomeAdLabel()
{
    m_settings->disableHomeAdLabel();
}

bool QAppSettingsRepository::isPremV1MigrationReminderActive() const
{
    return m_settings->isPremV1MigrationReminderActive();
}

void QAppSettingsRepository::disablePremV1MigrationReminder()
{
    m_settings->disablePremV1MigrationReminder();
}

QByteArray QAppSettingsRepository::backupAppConfig() const
{
    return m_settings->backupAppConfig();
}

bool QAppSettingsRepository::restoreAppConfig(const QByteArray &cfg)
{
    return m_settings->restoreAppConfig(cfg);
}

void QAppSettingsRepository::clearSettings()
{
    m_settings->clearSettings();
    emit settingsCleared();
}

QString QAppSettingsRepository::nextAvailableServerName() const
{
    return m_settings->nextAvailableServerName();
}

