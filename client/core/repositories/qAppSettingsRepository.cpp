#include "qAppSettingsRepository.h"

#include "core/defs.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "settings.h"

using namespace amnezia;

QAppSettingsRepository::QAppSettingsRepository(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_secureRepository(std::make_unique<SecureAppSettingsRepository>(settings))
{
}

QAppSettingsRepository::~QAppSettingsRepository() = default;

AppSettingsRepository* QAppSettingsRepository::repository()
{
    return m_secureRepository.get();
}

QLocale QAppSettingsRepository::getAppLanguage() const
{
    return m_secureRepository->getAppLanguage();
}

void QAppSettingsRepository::setAppLanguage(QLocale locale)
{
    m_secureRepository->setAppLanguage(locale);
    emit appLanguageChanged(locale);
}

bool QAppSettingsRepository::useAmneziaDns() const
{
    return m_secureRepository->useAmneziaDns();
}

void QAppSettingsRepository::setUseAmneziaDns(bool enabled)
{
    m_secureRepository->setUseAmneziaDns(enabled);
    emit useAmneziaDnsChanged(enabled);
}

QStringList QAppSettingsRepository::getAllowedDnsServers() const
{
    return m_secureRepository->getAllowedDnsServers();
}

void QAppSettingsRepository::setAllowedDnsServers(const QStringList &servers)
{
    m_secureRepository->setAllowedDnsServers(servers);
    emit allowedDnsServersChanged(servers);
}

QString QAppSettingsRepository::primaryDns() const
{
    return m_secureRepository->primaryDns();
}

void QAppSettingsRepository::setPrimaryDns(const QString &dns)
{
    m_secureRepository->setPrimaryDns(dns);
}

QString QAppSettingsRepository::secondaryDns() const
{
    return m_secureRepository->secondaryDns();
}

void QAppSettingsRepository::setSecondaryDns(const QString &dns)
{
    m_secureRepository->setSecondaryDns(dns);
}

RouteMode QAppSettingsRepository::routeMode() const
{
    return m_secureRepository->routeMode();
}

void QAppSettingsRepository::setRouteMode(RouteMode mode)
{
    m_secureRepository->setRouteMode(mode);
    emit routeModeChanged(mode);
}

bool QAppSettingsRepository::addVpnSite(RouteMode mode, const QString &site, const QString &ip)
{
    bool result = m_secureRepository->addVpnSite(mode, site, ip);
    if (result) {
        emit sitesChanged(mode);
    }
    return result;
}

void QAppSettingsRepository::addVpnSites(RouteMode mode, const QMap<QString, QString> &sites)
{
    m_secureRepository->addVpnSites(mode, sites);
    emit sitesChanged(mode);
}

void QAppSettingsRepository::removeVpnSite(RouteMode mode, const QString &site)
{
    m_secureRepository->removeVpnSite(mode, site);
    emit sitesChanged(mode);
}

void QAppSettingsRepository::removeAllVpnSites(RouteMode mode)
{
    m_secureRepository->removeAllVpnSites(mode);
    emit sitesChanged(mode);
}

QVariantMap QAppSettingsRepository::vpnSites(RouteMode mode) const
{
    return m_secureRepository->vpnSites(mode);
}

bool QAppSettingsRepository::isSitesSplitTunnelingEnabled() const
{
    return m_secureRepository->isSitesSplitTunnelingEnabled();
}

void QAppSettingsRepository::setSitesSplitTunnelingEnabled(bool enabled)
{
    m_secureRepository->setSitesSplitTunnelingEnabled(enabled);
    emit sitesSplitTunnelingEnabledChanged(enabled);
}

AppsRouteMode QAppSettingsRepository::appsRouteMode() const
{
    return m_secureRepository->appsRouteMode();
}

void QAppSettingsRepository::setAppsRouteMode(AppsRouteMode mode)
{
    m_secureRepository->setAppsRouteMode(mode);
    emit appsRouteModeChanged(mode);
}

void QAppSettingsRepository::setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps)
{
    m_secureRepository->setVpnApps(mode, apps);
    emit appsChanged(mode);
}

QVector<InstalledAppInfo> QAppSettingsRepository::vpnApps(AppsRouteMode mode) const
{
    return m_secureRepository->vpnApps(mode);
}

bool QAppSettingsRepository::isAppsSplitTunnelingEnabled() const
{
    return m_secureRepository->isAppsSplitTunnelingEnabled();
}

void QAppSettingsRepository::setAppsSplitTunnelingEnabled(bool enabled)
{
    m_secureRepository->setAppsSplitTunnelingEnabled(enabled);
    emit appsSplitTunnelingEnabledChanged(enabled);
}

QString QAppSettingsRepository::getGatewayEndpoint() const
{
    return m_secureRepository->getGatewayEndpoint();
}

void QAppSettingsRepository::setGatewayEndpoint(const QString &endpoint)
{
    m_secureRepository->setGatewayEndpoint(endpoint);
}

void QAppSettingsRepository::resetGatewayEndpoint()
{
    m_secureRepository->resetGatewayEndpoint();
}

void QAppSettingsRepository::setDevGatewayEndpoint()
{
    m_secureRepository->setDevGatewayEndpoint();
}

bool QAppSettingsRepository::isDevGatewayEnv() const
{
    return m_secureRepository->isDevGatewayEnv();
}

void QAppSettingsRepository::toggleDevGatewayEnv(bool enabled)
{
    m_secureRepository->toggleDevGatewayEnv(enabled);
}

bool QAppSettingsRepository::isKillSwitchEnabled() const
{
    return m_secureRepository->isKillSwitchEnabled();
}

void QAppSettingsRepository::setKillSwitchEnabled(bool enabled)
{
    m_secureRepository->setKillSwitchEnabled(enabled);
}

bool QAppSettingsRepository::isStrictKillSwitchEnabled() const
{
    return m_secureRepository->isStrictKillSwitchEnabled();
}

void QAppSettingsRepository::setStrictKillSwitchEnabled(bool enabled)
{
    m_secureRepository->setStrictKillSwitchEnabled(enabled);
}

bool QAppSettingsRepository::isAutoConnect() const
{
    return m_secureRepository->isAutoConnect();
}

void QAppSettingsRepository::setAutoConnect(bool enabled)
{
    m_secureRepository->setAutoConnect(enabled);
}

bool QAppSettingsRepository::isStartMinimized() const
{
    return m_secureRepository->isStartMinimized();
}

void QAppSettingsRepository::setStartMinimized(bool enabled)
{
    m_secureRepository->setStartMinimized(enabled);
}

bool QAppSettingsRepository::isScreenshotsEnabled() const
{
    return m_secureRepository->isScreenshotsEnabled();
}

void QAppSettingsRepository::setScreenshotsEnabled(bool enabled)
{
    m_secureRepository->setScreenshotsEnabled(enabled);
    emit screenshotsEnabledChanged(enabled);
}

bool QAppSettingsRepository::isSaveLogs() const
{
    return m_secureRepository->isSaveLogs();
}

void QAppSettingsRepository::setSaveLogs(bool enabled)
{
    m_secureRepository->setSaveLogs(enabled);
    emit saveLogsChanged(enabled);
}

QDateTime QAppSettingsRepository::getLogEnableDate() const
{
    return m_secureRepository->getLogEnableDate();
}

void QAppSettingsRepository::setLogEnableDate(const QDateTime &date)
{
    m_secureRepository->setLogEnableDate(date);
}

QString QAppSettingsRepository::getInstallationUuid(bool createIfNotExists) const
{
    return m_secureRepository->getInstallationUuid(createIfNotExists);
}

bool QAppSettingsRepository::isHomeAdLabelVisible() const
{
    return m_secureRepository->isHomeAdLabelVisible();
}

void QAppSettingsRepository::disableHomeAdLabel()
{
    m_secureRepository->disableHomeAdLabel();
}

bool QAppSettingsRepository::isPremV1MigrationReminderActive() const
{
    return m_secureRepository->isPremV1MigrationReminderActive();
}

void QAppSettingsRepository::disablePremV1MigrationReminder()
{
    m_secureRepository->disablePremV1MigrationReminder();
}

QByteArray QAppSettingsRepository::backupAppConfig() const
{
    return m_secureRepository->backupAppConfig();
}

bool QAppSettingsRepository::restoreAppConfig(const QByteArray &cfg)
{
    return m_secureRepository->restoreAppConfig(cfg);
}

void QAppSettingsRepository::clearSettings()
{
    m_secureRepository->clearSettings();
    emit settingsCleared();
}

QString QAppSettingsRepository::nextAvailableServerName() const
{
    return m_secureRepository->nextAvailableServerName();
}

