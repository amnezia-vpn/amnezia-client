#include "secureAppSettingsRepository.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/networkUtilities.h"

using namespace amnezia;

namespace {
    constexpr char gatewayEndpoint[] = "http://gw.amnezia.org:80/";
}

SecureAppSettingsRepository::SecureAppSettingsRepository(SecureQSettings* settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
    QString storedEndpoint = value("Conf/gatewayEndpoint", gatewayEndpoint).toString();
    m_gatewayEndpoint = storedEndpoint.isEmpty() ? gatewayEndpoint : storedEndpoint;
}

QVariant SecureAppSettingsRepository::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void SecureAppSettingsRepository::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
}

QLocale SecureAppSettingsRepository::getAppLanguage() const
{
    QString localeStr = value("Conf/appLanguage", QLocale::system().name()).toString();
    return QLocale(localeStr);
}

void SecureAppSettingsRepository::setAppLanguage(QLocale locale)
{
    setValue("Conf/appLanguage", locale.name());
    emit appLanguageChanged(locale);
}

bool SecureAppSettingsRepository::useAmneziaDns() const
{
    return value("Conf/useAmneziaDns", true).toBool();
}

void SecureAppSettingsRepository::setUseAmneziaDns(bool enabled)
{
    setValue("Conf/useAmneziaDns", enabled);
    emit useAmneziaDnsChanged(enabled);
}

QStringList SecureAppSettingsRepository::getAllowedDnsServers() const
{
    return value("Conf/allowedDnsServers").toStringList();
}

void SecureAppSettingsRepository::setAllowedDnsServers(const QStringList &servers)
{
    setValue("Conf/allowedDnsServers", servers);
    emit allowedDnsServersChanged(servers);
}

QString SecureAppSettingsRepository::primaryDns() const
{
    constexpr char cloudFlareNs1[] = "1.1.1.1";
    return value("Conf/primaryDns", cloudFlareNs1).toString();
}

void SecureAppSettingsRepository::setPrimaryDns(const QString &dns)
{
    setValue("Conf/primaryDns", dns);
}

QString SecureAppSettingsRepository::secondaryDns() const
{
    constexpr char cloudFlareNs2[] = "1.0.0.1";
    return value("Conf/secondaryDns", cloudFlareNs2).toString();
}

void SecureAppSettingsRepository::setSecondaryDns(const QString &dns)
{
    setValue("Conf/secondaryDns", dns);
}

namespace {
    QString routeModeString(RouteMode mode) {
        switch (mode) {
        case RouteMode::VpnAllSites: return "AllSites";
        case RouteMode::VpnOnlyForwardSites: return "ForwardSites";
        case RouteMode::VpnAllExceptSites: return "ExceptSites";
        }
        return QString();
    }
}

RouteMode SecureAppSettingsRepository::routeMode() const
{
    return static_cast<RouteMode>(value("Conf/routeMode", 0).toInt());
}

void SecureAppSettingsRepository::setRouteMode(RouteMode mode)
{
    setValue("Conf/routeMode", static_cast<int>(mode));
    emit routeModeChanged(mode);
}

QVariantMap SecureAppSettingsRepository::vpnSites(RouteMode mode) const
{
    return value("Conf/" + routeModeString(mode)).toMap();
}

void SecureAppSettingsRepository::setVpnSites(RouteMode mode, const QVariantMap &sites)
{
    setValue("Conf/" + routeModeString(mode), sites);
}

bool SecureAppSettingsRepository::addVpnSite(RouteMode mode, const QString &site, const QString &ip)
{
    QVariantMap sites = vpnSites(mode);
    if (sites.contains(site) && ip.isEmpty())
        return false;

    sites.insert(site, ip);
    setVpnSites(mode, sites);
    emit sitesChanged(mode);
    return true;
}

void SecureAppSettingsRepository::addVpnSites(RouteMode mode, const QMap<QString, QString> &sites)
{
    QVariantMap allSites = vpnSites(mode);
    for (auto i = sites.constBegin(); i != sites.constEnd(); ++i) {
        const QString &site = i.key();
        const QString &ip = i.value();

        if (allSites.contains(site) && allSites.value(site) == ip)
            continue;

        allSites.insert(site, ip);
    }

    setVpnSites(mode, allSites);
    emit sitesChanged(mode);
}

void SecureAppSettingsRepository::removeVpnSite(RouteMode mode, const QString &site)
{
    QVariantMap sites = vpnSites(mode);
    if (!sites.contains(site))
        return;

    sites.remove(site);
    setVpnSites(mode, sites);
    emit sitesChanged(mode);
}

void SecureAppSettingsRepository::removeAllVpnSites(RouteMode mode)
{
    setVpnSites(mode, QVariantMap());
    emit sitesChanged(mode);
}

bool SecureAppSettingsRepository::isSitesSplitTunnelingEnabled() const
{
    return value("Conf/sitesSplitTunnelingEnabled", false).toBool();
}

void SecureAppSettingsRepository::setSitesSplitTunnelingEnabled(bool enabled)
{
    setValue("Conf/sitesSplitTunnelingEnabled", enabled);
    emit sitesSplitTunnelingEnabledChanged(enabled);
}

namespace {
    QString appsRouteModeString(AppsRouteMode mode) {
        switch (mode) {
        case AppsRouteMode::VpnAllApps: return "AllApps";
        case AppsRouteMode::VpnOnlyForwardApps: return "ForwardApps";
        case AppsRouteMode::VpnAllExceptApps: return "ExceptApps";
        }
        return QString();
    }
}

AppsRouteMode SecureAppSettingsRepository::appsRouteMode() const
{
    return static_cast<AppsRouteMode>(value("Conf/appsRouteMode", 0).toInt());
}

void SecureAppSettingsRepository::setAppsRouteMode(AppsRouteMode mode)
{
    setValue("Conf/appsRouteMode", static_cast<int>(mode));
    emit appsRouteModeChanged(mode);
}

QVector<InstalledAppInfo> SecureAppSettingsRepository::vpnApps(AppsRouteMode mode) const
{
    QVector<InstalledAppInfo> apps;
    auto appsArray = value("Conf/" + appsRouteModeString(mode)).toJsonArray();
    for (const auto &app : appsArray) {
        InstalledAppInfo appInfo;
        appInfo.appName = app.toObject().value("appName").toString();
        appInfo.packageName = app.toObject().value("packageName").toString();
        appInfo.appPath = app.toObject().value("appPath").toString();

        apps.push_back(appInfo);
    }
    return apps;
}

void SecureAppSettingsRepository::setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps)
{
    QJsonArray appsArray;
    for (const auto &app : apps) {
        QJsonObject appInfo;
        appInfo.insert("appName", app.appName);
        appInfo.insert("packageName", app.packageName);
        appInfo.insert("appPath", app.appPath);
        appsArray.push_back(appInfo);
    }
    setValue("Conf/" + appsRouteModeString(mode), appsArray);
    emit appsChanged(mode);
}

bool SecureAppSettingsRepository::isAppsSplitTunnelingEnabled() const
{
    return value("Conf/appsSplitTunnelingEnabled", false).toBool();
}

void SecureAppSettingsRepository::setAppsSplitTunnelingEnabled(bool enabled)
{
    setValue("Conf/appsSplitTunnelingEnabled", enabled);
    emit appsSplitTunnelingEnabledChanged(enabled);
}

QString SecureAppSettingsRepository::getGatewayEndpoint(bool isTestPurchase) const
{
    if (isTestPurchase) {
        return QString(DEV_AGW_ENDPOINT);
    }
    return m_gatewayEndpoint;
}

void SecureAppSettingsRepository::setGatewayEndpoint(const QString &endpoint)
{
    m_gatewayEndpoint = endpoint;
    setValue("Conf/gatewayEndpoint", endpoint);
}

void SecureAppSettingsRepository::resetGatewayEndpoint()
{
    m_gatewayEndpoint = gatewayEndpoint;
    setValue("Conf/gatewayEndpoint", gatewayEndpoint);
}

void SecureAppSettingsRepository::setDevGatewayEndpoint()
{
    m_gatewayEndpoint = QString(DEV_AGW_ENDPOINT);
    setValue("Conf/gatewayEndpoint", DEV_AGW_ENDPOINT);
}

bool SecureAppSettingsRepository::isDevGatewayEnv(bool isTestPurchase) const
{
    return isTestPurchase ? true : value("Conf/devGatewayEnv", false).toBool();
}

void SecureAppSettingsRepository::toggleDevGatewayEnv(bool enabled)
{
    setValue("Conf/devGatewayEnv", enabled);
}

bool SecureAppSettingsRepository::isKillSwitchEnabled() const
{
    return value("Conf/killSwitchEnabled", true).toBool();
}

void SecureAppSettingsRepository::setKillSwitchEnabled(bool enabled)
{
    setValue("Conf/killSwitchEnabled", enabled);
}

bool SecureAppSettingsRepository::isStrictKillSwitchEnabled() const
{
    return value("Conf/strictKillSwitchEnabled", false).toBool();
}

void SecureAppSettingsRepository::setStrictKillSwitchEnabled(bool enabled)
{
    setValue("Conf/strictKillSwitchEnabled", enabled);
}

bool SecureAppSettingsRepository::isAutoConnect() const
{
    return value("Conf/autoConnect", false).toBool();
}

void SecureAppSettingsRepository::setAutoConnect(bool enabled)
{
    setValue("Conf/autoConnect", enabled);
}

bool SecureAppSettingsRepository::isStartMinimized() const
{
    return value("Conf/startMinimized", false).toBool();
}

void SecureAppSettingsRepository::setStartMinimized(bool enabled)
{
    setValue("Conf/startMinimized", enabled);
}

bool SecureAppSettingsRepository::isScreenshotsEnabled() const
{
    return value("Conf/screenshotsEnabled", true).toBool();
}

void SecureAppSettingsRepository::setScreenshotsEnabled(bool enabled)
{
    setValue("Conf/screenshotsEnabled", enabled);
    emit screenshotsEnabledChanged(enabled);
}

bool SecureAppSettingsRepository::isNewsNotifications() const
{
    return value("Conf/newsNotifications", true).toBool();
}

void SecureAppSettingsRepository::setNewsNotifications(bool enabled)
{
    setValue("Conf/newsNotifications", enabled);
}

bool SecureAppSettingsRepository::isSaveLogs() const
{
    return value("Conf/saveLogs", false).toBool();
}

void SecureAppSettingsRepository::setSaveLogs(bool enabled)
{
    setValue("Conf/saveLogs", enabled);
    emit saveLogsChanged(enabled);
}

QDateTime SecureAppSettingsRepository::getLogEnableDate() const
{
    return value("Conf/logEnableDate").toDateTime();
}

void SecureAppSettingsRepository::setLogEnableDate(const QDateTime &date)
{
    setValue("Conf/logEnableDate", date);
}

QString SecureAppSettingsRepository::getInstallationUuid(bool createIfNotExists) const
{
    auto uuid = value("Conf/installationUuid", "").toString();
    if (createIfNotExists && uuid.isEmpty()) {
        uuid = QUuid::createUuid().toString();
        uuid.remove(0, 1);
        uuid.chop(1);
        const_cast<SecureAppSettingsRepository*>(this)->setValue("Conf/installationUuid", uuid);
    } else if (uuid.contains("{") && uuid.contains("}")) {
        uuid.remove(0, 1);
        uuid.chop(1);
        const_cast<SecureAppSettingsRepository*>(this)->setValue("Conf/installationUuid", uuid);
    }
    return uuid;
}

QStringList SecureAppSettingsRepository::getReadNewsIds() const
{
    return value("News/readIds").toStringList();
}

void SecureAppSettingsRepository::setReadNewsIds(const QStringList &ids)
{
    setValue("News/readIds", ids);
}

bool SecureAppSettingsRepository::isHomeAdLabelVisible() const
{
    return value("Conf/homeAdLabelVisible", true).toBool();
}

void SecureAppSettingsRepository::disableHomeAdLabel()
{
    setValue("Conf/homeAdLabelVisible", false);
}

bool SecureAppSettingsRepository::isPremV1MigrationReminderActive() const
{
    return value("Conf/premV1MigrationReminderActive", true).toBool();
}

void SecureAppSettingsRepository::disablePremV1MigrationReminder()
{
    setValue("Conf/premV1MigrationReminderActive", false);
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
    auto uuid = getInstallationUuid(false);
    m_settings->clearSettings();
    m_settings->setValue("Conf/installationUuid", uuid);
    emit settingsCleared();
}

void SecureAppSettingsRepository::setInstallationUuid(const QString &uuid)
{
    m_settings->setValue("Conf/installationUuid", uuid);
}

QByteArray SecureAppSettingsRepository::xraySavedConfigs() const
{
    return value("Xray/savedConfigs").toByteArray();
}

void SecureAppSettingsRepository::setXraySavedConfigs(const QByteArray &data)
{
    setValue("Xray/savedConfigs", data);
}
