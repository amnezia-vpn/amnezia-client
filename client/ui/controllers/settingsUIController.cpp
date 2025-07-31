#include "settingsUIController.h"

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

SettingsUIController::SettingsUIController(const QSharedPointer<ServersModel> &serversModel,
                                       const QSharedPointer<ContainersModel> &containersModel,
                                       const QSharedPointer<LanguageModel> &languageModel,
                                       const QSharedPointer<SitesModel> &sitesModel,
                                       const QSharedPointer<AppSplitTunnelingModel> &appSplitTunnelingModel,
                                       QSharedPointer<SettingsController> settingsController,
                                       QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_languageModel(languageModel),
      m_sitesModel(sitesModel),
      m_appSplitTunnelingModel(appSplitTunnelingModel),
      m_settingsController(settingsController)
{
    m_appVersion = QString("%1 (%2, %3)").arg(QString(APP_VERSION), __DATE__, GIT_COMMIT_HASH);
    checkIfNeedDisableLogs();
#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::notificationStateChanged, this, &SettingsUIController::onNotificationStateChanged);
#endif
}

void SettingsUIController::toggleAmneziaDns(bool enable)
{
    m_settingsController->toggleAmneziaDns(enable);
    emit amneziaDnsToggled(enable);
}

bool SettingsUIController::isAmneziaDnsEnabled()
{
    return m_settingsController->isAmneziaDnsEnabled();
}

QString SettingsUIController::getPrimaryDns()
{
    return m_settingsController->getPrimaryDns();
}

void SettingsUIController::setPrimaryDns(const QString &dns)
{
    m_settingsController->configureDns(dns, m_settingsController->getSecondaryDns());
    emit primaryDnsChanged();
}

QString SettingsUIController::getSecondaryDns()
{
    return m_settingsController->getSecondaryDns();
}

void SettingsUIController::setSecondaryDns(const QString &dns)
{
    m_settingsController->configureDns(m_settingsController->getPrimaryDns(), dns);
    emit secondaryDnsChanged();
}

bool SettingsUIController::isLoggingEnabled()
{
    return m_settingsController->isLoggingEnabled();
}

void SettingsUIController::toggleLogging(bool enable)
{
    m_settingsController->configureLogging(enable);
#ifdef Q_OS_IOS
    AmneziaVPN::toggleLogging(enable);
#endif
    if (enable == true) {
        qInfo().noquote() << QString("Logging has enabled on %1 version %2 %3").arg(APPLICATION_NAME, APP_VERSION, GIT_COMMIT_HASH);
        qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    }
    emit loggingStateChanged();
}

void SettingsUIController::openLogsFolder()
{
    Logger::openLogsFolder(false);
}

void SettingsUIController::openServiceLogsFolder()
{
    Logger::openLogsFolder(true);
}

void SettingsUIController::exportLogsFile(const QString &fileName)
{
    m_settingsController->exportLogsFile(fileName);
}

void SettingsUIController::exportServiceLogsFile(const QString &fileName)
{
    m_settingsController->exportServiceLogsFile(fileName);
}

void SettingsUIController::clearLogs()
{
    m_settingsController->clearLogs();
}

void SettingsUIController::backupAppConfig(const QString &fileName)
{
    m_settingsController->backupAppConfig(fileName);
}

void SettingsUIController::restoreAppConfig(const QString &fileName)
{
    bool success = m_settingsController->restoreAppConfig(fileName);
    if (success) {
        m_serversModel->resetModel();
        m_languageModel->changeLanguage(
                static_cast<LanguageSettings::AvailableLanguageEnum>(m_languageModel->getCurrentLanguageIndex()));
        emit restoreBackupFinished();
    } else {
        emit changeSettingsErrorOccurred(tr("Backup file is corrupted"));
    }
}

void SettingsUIController::restoreAppConfigFromData(const QByteArray &data)
{
    bool success = m_settingsController->restoreAppConfigFromData(data);
    if (success) {
        m_serversModel->resetModel();
        m_languageModel->changeLanguage(
                static_cast<LanguageSettings::AvailableLanguageEnum>(m_languageModel->getCurrentLanguageIndex()));
        emit restoreBackupFinished();
    } else {
        emit changeSettingsErrorOccurred(tr("Backup file is corrupted"));
    }
}

QString SettingsUIController::getAppVersion()
{
    return m_appVersion;
}

void SettingsUIController::clearSettings()
{
    m_settingsController->resetAllSettings();
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

bool SettingsUIController::isAutoConnectEnabled()
{
    return m_settingsController->isAutoConnectEnabled();
}

void SettingsUIController::toggleAutoConnect(bool enable)
{
    m_settingsController->configureAutoConnect(enable);
}

bool SettingsUIController::isAutoStartEnabled()
{
    return m_settingsController->isAutoStartEnabled();
}

void SettingsUIController::toggleAutoStart(bool enable)
{
    m_settingsController->configureAutoStart(enable);
}

bool SettingsUIController::isStartMinimizedEnabled()
{
    return m_settingsController->isStartMinimizedEnabled();
}

void SettingsUIController::toggleStartMinimized(bool enable)
{
    m_settingsController->configureStartMinimized(enable);
}

bool SettingsUIController::isScreenshotsEnabled()
{
    return m_settingsController->isScreenshotsEnabled();
}

void SettingsUIController::toggleScreenshotsEnabled(bool enable)
{
    m_settingsController->configureScreenshots(enable);
}

bool SettingsUIController::isCameraPresent()
{
#if defined Q_OS_IOS
    return true;
#elif defined Q_OS_ANDROID
    return AndroidController::instance()->isCameraPresent();
#else
    return false;
#endif
}

void SettingsUIController::checkIfNeedDisableLogs()
{
    if (m_settingsController->isLoggingEnabled()) {
        m_loggingDisableDate = m_settingsController->getLogEnableDate().addDays(14);
        if (m_loggingDisableDate <= QDateTime::currentDateTime()) {
            toggleLogging(false);
            clearLogs();
            emit loggingDisableByWatcher();
        }
    }
}

bool SettingsUIController::isKillSwitchEnabled()
{
    return m_settingsController->isKillSwitchEnabled();
}

void SettingsUIController::toggleKillSwitch(bool enable)
{
    m_settingsController->configureKillSwitch(enable, false);
    emit killSwitchEnabledChanged();
    if (enable == false) {
        emit strictKillSwitchEnabledChanged(false);
    } else {
        emit strictKillSwitchEnabledChanged(isStrictKillSwitchEnabled());
    }
}

bool SettingsUIController::isStrictKillSwitchEnabled()
{
    return m_settingsController->isStrictKillSwitchEnabled();
}

void SettingsUIController::toggleStrictKillSwitch(bool enable)
{
    m_settingsController->configureKillSwitch(m_settingsController->isKillSwitchEnabled(), enable);
    emit strictKillSwitchEnabledChanged(enable);
}

bool SettingsUIController::isNotificationPermissionGranted()
{
#ifdef Q_OS_ANDROID
    return AndroidController::instance()->isNotificationPermissionGranted();
#else
    return true;
#endif
}

void SettingsUIController::requestNotificationPermission()
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->requestNotificationPermission();
#endif
}

QString SettingsUIController::getInstallationUuid()
{
    return m_settingsController->getInstallationUuid();
}

void SettingsUIController::enableDevMode()
{
    m_isDevModeEnabled = true;
    emit devModeEnabled();
}

bool SettingsUIController::isDevModeEnabled()
{
    return m_isDevModeEnabled;
}

void SettingsUIController::resetGatewayEndpoint()
{
    m_settingsController->resetGatewayEndpoint();
    emit gatewayEndpointChanged(m_settingsController->getGatewayEndpoint());
}

void SettingsUIController::setGatewayEndpoint(const QString &endpoint)
{
    m_settingsController->setGatewayEndpoint(endpoint);
    emit gatewayEndpointChanged(endpoint);
}

QString SettingsUIController::getGatewayEndpoint()
{
    return m_settingsController->getGatewayEndpoint();
}

bool SettingsUIController::isDevGatewayEnv()
{
    return m_settingsController->isDevGatewayEnv();
}

void SettingsUIController::toggleDevGatewayEnv(bool enabled)
{
    m_settingsController->toggleDevGatewayEnv(enabled);
    emit gatewayEndpointChanged(m_settingsController->getGatewayEndpoint());
    emit devGatewayEnvChanged(enabled);
}

bool SettingsUIController::isOnTv()
{
#ifdef Q_OS_ANDROID
    return AndroidController::instance()->isOnTv();
#else
    return false;
#endif
}

bool SettingsUIController::isHomeAdLabelVisible()
{
    return m_settingsController->isHomeAdLabelVisible();
}

void SettingsUIController::disableHomeAdLabel()
{
    m_settingsController->disableHomeAdLabel();
    emit isHomeAdLabelVisibleChanged(false);
}
