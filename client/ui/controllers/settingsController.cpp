#include "settingsController.h"

#include <QStandardPaths>

#include "logger.h"
#include "systemController.h"
#include "ui/qautostart.h"
#include "version.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#ifdef Q_OS_IOS
    #include <AmneziaVPN-Swift.h>
#endif

SettingsController::SettingsController(const QSharedPointer<ServersModel> &serversModel,
                                       const QSharedPointer<ContainersModel> &containersModel,
                                       const QSharedPointer<LanguageModel> &languageModel,
                                       const QSharedPointer<SitesModel> &sitesModel,
                                       const QSharedPointer<AppSplitTunnelingModel> &appSplitTunnelingModel,
                                       QSharedPointer<SettingsConfigController> settingsConfigController,
                                       QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_languageModel(languageModel),
      m_sitesModel(sitesModel),
      m_appSplitTunnelingModel(appSplitTunnelingModel),
      m_settingsConfigController(settingsConfigController)
{
    m_appVersion = QString("%1 (%2, %3)").arg(QString(APP_VERSION), __DATE__, GIT_COMMIT_HASH);
    checkIfNeedDisableLogs();
#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::notificationStateChanged, this, &SettingsController::onNotificationStateChanged);
#endif
}

void SettingsController::toggleAmneziaDns(bool enable)
{
    m_settingsConfigController->toggleAmneziaDns(enable);
    emit amneziaDnsToggled(enable);
}

bool SettingsController::isAmneziaDnsEnabled()
{
    return m_settingsConfigController->isAmneziaDnsEnabled();
}

QString SettingsController::getPrimaryDns()
{
    return m_settingsConfigController->getPrimaryDns();
}

void SettingsController::setPrimaryDns(const QString &dns)
{
    m_settingsConfigController->configureDns(dns, m_settingsConfigController->getSecondaryDns());
    emit primaryDnsChanged();
}

QString SettingsController::getSecondaryDns()
{
    return m_settingsConfigController->getSecondaryDns();
}

void SettingsController::setSecondaryDns(const QString &dns)
{
    m_settingsConfigController->configureDns(m_settingsConfigController->getPrimaryDns(), dns);
    emit secondaryDnsChanged();
}

bool SettingsController::isLoggingEnabled()
{
    return m_settingsConfigController->isLoggingEnabled();
}

void SettingsController::toggleLogging(bool enable)
{
    m_settingsConfigController->configureLogging(enable);
#ifdef Q_OS_IOS
    AmneziaVPN::toggleLogging(enable);
#endif
    if (enable == true) {
        qInfo().noquote() << QString("Logging has enabled on %1 version %2 %3").arg(APPLICATION_NAME, APP_VERSION, GIT_COMMIT_HASH);
        qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    }
    emit loggingStateChanged();
}

void SettingsController::openLogsFolder()
{
    Logger::openLogsFolder(false);
}

void SettingsController::openServiceLogsFolder()
{
    Logger::openLogsFolder(true);
}

void SettingsController::exportLogsFile(const QString &fileName)
{
    m_settingsConfigController->exportLogsFile(fileName);
}

void SettingsController::exportServiceLogsFile(const QString &fileName)
{
    m_settingsConfigController->exportServiceLogsFile(fileName);
}

void SettingsController::clearLogs()
{
    m_settingsConfigController->clearLogs();
}

void SettingsController::backupAppConfig(const QString &fileName)
{
    m_settingsConfigController->backupAppConfig(fileName);
}

void SettingsController::restoreAppConfig(const QString &fileName)
{
    bool success = m_settingsConfigController->restoreAppConfig(fileName);
    if (success) {
        m_serversModel->resetModel();
        m_languageModel->changeLanguage(
                static_cast<LanguageSettings::AvailableLanguageEnum>(m_languageModel->getCurrentLanguageIndex()));
        emit restoreBackupFinished();
    } else {
        emit changeSettingsErrorOccurred(tr("Backup file is corrupted"));
    }
}

void SettingsController::restoreAppConfigFromData(const QByteArray &data)
{
    bool success = m_settingsConfigController->restoreAppConfigFromData(data);
    if (success) {
        m_serversModel->resetModel();
        m_languageModel->changeLanguage(
                static_cast<LanguageSettings::AvailableLanguageEnum>(m_languageModel->getCurrentLanguageIndex()));
        emit restoreBackupFinished();
    } else {
        emit changeSettingsErrorOccurred(tr("Backup file is corrupted"));
    }
}

QString SettingsController::getAppVersion()
{
    return m_appVersion;
}

void SettingsController::clearSettings()
{
    m_settingsConfigController->resetAllSettings();
    m_serversModel->resetModel();
    m_languageModel->changeLanguage(
            static_cast<LanguageSettings::AvailableLanguageEnum>(m_languageModel->getCurrentLanguageIndex()));

    m_sitesModel->setRouteMode(Settings::RouteMode::VpnOnlyForwardSites);
    m_sitesModel->toggleSplitTunneling(false);

    m_appSplitTunnelingModel->setRouteMode(Settings::AppsRouteMode::VpnAllExceptApps);
    m_appSplitTunnelingModel->toggleSplitTunneling(false);

    toggleAutoStart(false);

    emit changeSettingsFinished(tr("All settings have been reset to default values"));

#ifdef Q_OS_IOS
    AmneziaVPN::clearSettings();
#endif
}

bool SettingsController::isAutoConnectEnabled()
{
    return m_settingsConfigController->isAutoConnectEnabled();
}

void SettingsController::toggleAutoConnect(bool enable)
{
    m_settingsConfigController->configureAutoConnect(enable);
}

bool SettingsController::isAutoStartEnabled()
{
    return m_settingsConfigController->isAutoStartEnabled();
}

void SettingsController::toggleAutoStart(bool enable)
{
    m_settingsConfigController->configureAutoStart(enable);
}

bool SettingsController::isStartMinimizedEnabled()
{
    return m_settingsConfigController->isStartMinimizedEnabled();
}

void SettingsController::toggleStartMinimized(bool enable)
{
    m_settingsConfigController->configureStartMinimized(enable);
}

bool SettingsController::isScreenshotsEnabled()
{
    return m_settingsConfigController->isScreenshotsEnabled();
}

void SettingsController::toggleScreenshotsEnabled(bool enable)
{
    m_settingsConfigController->configureScreenshots(enable);
}

bool SettingsController::isCameraPresent()
{
#if defined Q_OS_IOS
    return true;
#elif defined Q_OS_ANDROID
    return AndroidController::instance()->isCameraPresent();
#else
    return false;
#endif
}

void SettingsController::checkIfNeedDisableLogs()
{
    if (m_settingsConfigController->isLoggingEnabled()) {
        m_loggingDisableDate = m_settingsConfigController->getLogEnableDate().addDays(14);
        if (m_loggingDisableDate <= QDateTime::currentDateTime()) {
            toggleLogging(false);
            clearLogs();
            emit loggingDisableByWatcher();
        }
    }
}

bool SettingsController::isKillSwitchEnabled()
{
    return m_settingsConfigController->isKillSwitchEnabled();
}

void SettingsController::toggleKillSwitch(bool enable)
{
    m_settingsConfigController->configureKillSwitch(enable, false);
    emit killSwitchEnabledChanged();
    if (enable == false) {
        emit strictKillSwitchEnabledChanged(false);
    } else {
        emit strictKillSwitchEnabledChanged(isStrictKillSwitchEnabled());
    }
}

bool SettingsController::isStrictKillSwitchEnabled()
{
    return m_settingsConfigController->isStrictKillSwitchEnabled();
}

void SettingsController::toggleStrictKillSwitch(bool enable)
{
    m_settingsConfigController->configureKillSwitch(m_settingsConfigController->isKillSwitchEnabled(), enable);
    emit strictKillSwitchEnabledChanged(enable);
}

bool SettingsController::isNotificationPermissionGranted()
{
#ifdef Q_OS_ANDROID
    return AndroidController::instance()->isNotificationPermissionGranted();
#else
    return true;
#endif
}

void SettingsController::requestNotificationPermission()
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->requestNotificationPermission();
#endif
}

QString SettingsController::getInstallationUuid()
{
    return m_settingsConfigController->getInstallationUuid();
}

void SettingsController::enableDevMode()
{
    m_isDevModeEnabled = true;
    emit devModeEnabled();
}

bool SettingsController::isDevModeEnabled()
{
    return m_isDevModeEnabled;
}

void SettingsController::resetGatewayEndpoint()
{
    m_settingsConfigController->resetGatewayEndpoint();
    emit gatewayEndpointChanged(m_settingsConfigController->getGatewayEndpoint());
}

void SettingsController::setGatewayEndpoint(const QString &endpoint)
{
    m_settingsConfigController->setGatewayEndpoint(endpoint);
    emit gatewayEndpointChanged(endpoint);
}

QString SettingsController::getGatewayEndpoint()
{
    return m_settingsConfigController->getGatewayEndpoint();
}

bool SettingsController::isDevGatewayEnv()
{
    return m_settingsConfigController->isDevGatewayEnv();
}

void SettingsController::toggleDevGatewayEnv(bool enabled)
{
    m_settingsConfigController->toggleDevGatewayEnv(enabled);
    emit gatewayEndpointChanged(m_settingsConfigController->getGatewayEndpoint());
    emit devGatewayEnvChanged(enabled);
}

bool SettingsController::isOnTv()
{
#ifdef Q_OS_ANDROID
    return AndroidController::instance()->isOnTv();
#else
    return false;
#endif
}

bool SettingsController::isHomeAdLabelVisible()
{
    return m_settingsConfigController->isHomeAdLabelVisible();
}

void SettingsController::disableHomeAdLabel()
{
    m_settingsConfigController->disableHomeAdLabel();
    emit isHomeAdLabelVisibleChanged(false);
}
