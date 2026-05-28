#include "settingsController.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOperatingSystemVersion>

#include "version.h"
#include "ui/utils/qAutoStart.h"
#include "logger.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

QString getPlatformName()
{
#if defined(Q_OS_WINDOWS)
    return "Windows";
#elif defined(Q_OS_ANDROID)
    return "Android";
#elif defined(Q_OS_LINUX)
    return "Linux";
#elif defined(Q_OS_MACX)
    return "MacOS";
#elif defined(Q_OS_IOS)
    return "iOS";
#else
    return "Unknown";
#endif
}

SettingsController::SettingsController(SecureServersRepository* serversRepository,
                                     SecureAppSettingsRepository* appSettingsRepository,
                                     QObject* parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository)
{
    m_appVersion = QString("%1 (%2, %3)").arg(QString(APP_VERSION), __DATE__, GIT_COMMIT_HASH);
    m_isDevModeEnabled = m_appSettingsRepository->isDevGatewayEnv();
}

void SettingsController::toggleAmneziaDns(bool enable)
{
    m_appSettingsRepository->setUseAmneziaDns(enable);
}

bool SettingsController::isAmneziaDnsEnabled() const
{
    return m_appSettingsRepository->useAmneziaDns();
}

QString SettingsController::getPrimaryDns() const
{
    return m_appSettingsRepository->primaryDns();
}

void SettingsController::setPrimaryDns(const QString &dns)
{
    m_appSettingsRepository->setPrimaryDns(dns);
}

QString SettingsController::getSecondaryDns() const
{
    return m_appSettingsRepository->secondaryDns();
}

void SettingsController::setSecondaryDns(const QString &dns)
{
    m_appSettingsRepository->setSecondaryDns(dns);
}

bool SettingsController::isLoggingEnabled() const
{
    return m_appSettingsRepository->isSaveLogs();
}

void SettingsController::toggleLogging(bool enable)
{
    m_appSettingsRepository->setSaveLogs(enable);
#ifndef Q_OS_ANDROID
    if (!enable) {
        Logger::deInit();
    } else {
        if (!Logger::init(false)) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }
#endif
    Logger::setServiceLogsEnabled(enable);

    if (enable) {
        m_appSettingsRepository->setLogEnableDate(QDateTime::currentDateTime());
    }
}

void SettingsController::clearLogs()
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->clearLogs();
#else
    Logger::clearLogs(false);
    Logger::clearServiceLogs();
#endif
}

QByteArray SettingsController::backupAppConfig() const
{
    QByteArray data = m_appSettingsRepository->backupAppConfig();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject config = doc.object();

    config["AppPlatform"] = getPlatform();
    config["Conf/autoStart"] = isAutoStartEnabled();
    config["Conf/killSwitchEnabled"] = isKillSwitchEnabled();
    config["Conf/strictKillSwitchEnabled"] = isStrictKillSwitchEnabled();
    config["Conf/useAmneziaDns"] = isAmneziaDnsEnabled();

    return QJsonDocument(config).toJson();
}

ErrorCode SettingsController::restoreAppConfigFromData(const QByteArray &data)
{
    if (!m_appSettingsRepository->restoreAppConfig(data)) {
        return ErrorCode::RestoreBackupInvalidError;
    }

    m_serversRepository->invalidateCache();

    QJsonObject newConfigData = QJsonDocument::fromJson(data).object();

#if defined(Q_OS_WINDOWS) || defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    bool autoStart = false;
    if (newConfigData.contains("Conf/autoStart")) {
        autoStart = newConfigData["Conf/autoStart"].toBool();
    }
    toggleAutoStart(autoStart);
#endif

#if defined(Q_OS_WINDOWS) || defined(Q_OS_ANDROID)
    int appSplitTunnelingRouteMode = newConfigData.value("Conf/appsRouteMode").toInt();
    bool appSplittunnelingEnabled =
            newConfigData.value("Conf/appsSplitTunnelingEnabled").toVariant().toString().toLower() == "true";
    emit appSplitTunnelingRouteModeChanged(static_cast<AppsRouteMode>(appSplitTunnelingRouteMode));

    #if defined(Q_OS_WINDOWS)
        emit appSplitTunnelingRouteModeChanged(AppsRouteMode::VpnAllExceptApps);
    #endif

    if (newConfigData.contains("AppPlatform")) {
            if (newConfigData.value("AppPlatform").toString() != getPlatform()) {
                emit appSplitTunnelingClearAppsList();
            }
    }
    
    emit appSplitTunnelingToggled(appSplittunnelingEnabled);
#endif

    int siteSplitTunnelingRouteMode = newConfigData.value("Conf/routeMode").toInt();
    bool siteSplittunnelingEnabled =
            newConfigData.value("Conf/sitesSplitTunnelingEnabled").toVariant().toString().toLower() == "true";
    emit siteSplitTunnelingRouteModeChanged(static_cast<RouteMode>(siteSplitTunnelingRouteMode));
    emit siteSplitTunnelingToggled(siteSplittunnelingEnabled);

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    m_appSettingsRepository->setAutoConnect(false);
    m_appSettingsRepository->setStartMinimized(false);
    m_appSettingsRepository->setKillSwitchEnabled(false);
    m_appSettingsRepository->setStrictKillSwitchEnabled(false);
#endif

    return ErrorCode::NoError;
}

QString SettingsController::getAppVersion() const
{
    return m_appVersion;
}

void SettingsController::clearSettings()
{
    m_appSettingsRepository->clearSettings();

    m_serversRepository->clearServers();

    emit siteSplitTunnelingRouteModeChanged(RouteMode::VpnOnlyForwardSites);
    emit siteSplitTunnelingToggled(false);

    emit appSplitTunnelingRouteModeChanged(AppsRouteMode::VpnAllExceptApps);
    emit appSplitTunnelingToggled(false);

    toggleAutoStart(false);
}

bool SettingsController::isAutoConnectEnabled() const
{
    return m_appSettingsRepository->isAutoConnect();
}

void SettingsController::toggleAutoConnect(bool enable)
{
    m_appSettingsRepository->setAutoConnect(enable);
}

bool SettingsController::isAutoStartEnabled() const
{
    return Autostart::isAutostart();
}

void SettingsController::toggleAutoStart(bool enable)
{
    Autostart::setAutostart(enable);
    if (!enable) {
        toggleStartMinimized(false);
    }
}

bool SettingsController::isStartMinimizedEnabled() const
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (!isAutoStartEnabled()) {
        return false;
    }
#endif
    return m_appSettingsRepository->isStartMinimized();
}

void SettingsController::toggleStartMinimized(bool enable)
{
    m_appSettingsRepository->setStartMinimized(enable);
}

bool SettingsController::isScreenshotsEnabled() const
{
    return m_appSettingsRepository->isScreenshotsEnabled();
}

void SettingsController::toggleScreenshotsEnabled(bool enable)
{
    m_appSettingsRepository->setScreenshotsEnabled(enable);
}

bool SettingsController::isNewsNotificationsEnabled() const
{
    return m_appSettingsRepository->isNewsNotifications();
}

void SettingsController::toggleNewsNotificationsEnabled(bool enable)
{
    m_appSettingsRepository->setNewsNotifications(enable);
}

bool SettingsController::isKillSwitchEnabled() const
{
    return m_appSettingsRepository->isKillSwitchEnabled();
}

void SettingsController::toggleKillSwitch(bool enable)
{
    m_appSettingsRepository->setKillSwitchEnabled(enable);
}

bool SettingsController::isStrictKillSwitchEnabled() const
{
    return m_appSettingsRepository->isStrictKillSwitchEnabled();
}

void SettingsController::toggleStrictKillSwitch(bool enable)
{
    m_appSettingsRepository->setStrictKillSwitchEnabled(enable);
}

QString SettingsController::getInstallationUuid(bool createIfNotExists) const
{
    return m_appSettingsRepository->getInstallationUuid(createIfNotExists);
}

void SettingsController::enableDevMode()
{
    m_isDevModeEnabled = true;
}

bool SettingsController::isDevModeEnabled() const
{
    return m_isDevModeEnabled;
}

void SettingsController::resetGatewayEndpoint()
{
    m_appSettingsRepository->resetGatewayEndpoint();
}

void SettingsController::setGatewayEndpoint(const QString &endpoint)
{
    m_appSettingsRepository->setGatewayEndpoint(endpoint);
}

QString SettingsController::getGatewayEndpoint() const
{
    return m_appSettingsRepository->isDevGatewayEnv() ? "Dev endpoint" : m_appSettingsRepository->getGatewayEndpoint();
}

bool SettingsController::isDevGatewayEnv() const
{
    return m_appSettingsRepository->isDevGatewayEnv();
}

void SettingsController::toggleDevGatewayEnv(bool enabled)
{
    m_appSettingsRepository->toggleDevGatewayEnv(enabled);
    if (enabled) {
        m_appSettingsRepository->setDevGatewayEndpoint();
    } else {
        m_appSettingsRepository->resetGatewayEndpoint();
    }
}

bool SettingsController::isHomeAdLabelVisible() const
{
    return m_appSettingsRepository->isHomeAdLabelVisible();
}

void SettingsController::disableHomeAdLabel()
{
    m_appSettingsRepository->disableHomeAdLabel();
}

void SettingsController::checkIfNeedDisableLogs()
{
    if (m_appSettingsRepository->isSaveLogs()) {
        m_loggingDisableDate = m_appSettingsRepository->getLogEnableDate().addDays(14);
        if (m_loggingDisableDate <= QDateTime::currentDateTime()) {
            toggleLogging(false);
            clearLogs();
        }
    }
}

QString SettingsController::getPlatform() const
{
    return getPlatformName();
}

QLocale SettingsController::getAppLanguage() const
{
    return m_appSettingsRepository->getAppLanguage();
}

void SettingsController::setAppLanguage(const QLocale &locale)
{
    m_appSettingsRepository->setAppLanguage(locale);
}

bool SettingsController::isPremV1MigrationReminderActive() const
{
    return m_appSettingsRepository->isPremV1MigrationReminderActive();
}

void SettingsController::disablePremV1MigrationReminder()
{
    m_appSettingsRepository->disablePremV1MigrationReminder();
}

QString SettingsController::nextAvailableServerName() const
{
    return m_serversRepository->nextAvailableServerName();
}

