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
                                       const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_languageModel(languageModel),
      m_sitesModel(sitesModel),
      m_appSplitTunnelingModel(appSplitTunnelingModel),
      m_settings(settings)
{
    m_appVersion = QString("%1 (%2, %3)").arg(QString(APP_VERSION), __DATE__, GIT_COMMIT_HASH);
    checkIfNeedDisableLogs();
#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::notificationStateChanged, this, &SettingsController::onNotificationStateChanged);
#endif
}

void SettingsController::toggleAmneziaDns(bool enable)
{
    m_settings->setUseAmneziaDns(enable);
    emit amneziaDnsToggled(enable);
}

bool SettingsController::isAmneziaDnsEnabled()
{
    return m_settings->useAmneziaDns();
}

QString SettingsController::getPrimaryDns()
{
    return m_settings->primaryDns();
}

void SettingsController::setPrimaryDns(const QString &dns)
{
    m_settings->setPrimaryDns(dns);
    emit primaryDnsChanged();
}

QString SettingsController::getSecondaryDns()
{
    return m_settings->secondaryDns();
}

void SettingsController::setSecondaryDns(const QString &dns)
{
    return m_settings->setSecondaryDns(dns);
    emit secondaryDnsChanged();
}

bool SettingsController::isLoggingEnabled()
{
    return m_settings->isSaveLogs();
}

void SettingsController::toggleLogging(bool enable)
{
    m_settings->setSaveLogs(enable);
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
#ifdef Q_OS_ANDROID
    AndroidController::instance()->exportLogsFile(fileName);
#else
    SystemController::saveFile(fileName, Logger::getLogFile());
#endif
}

void SettingsController::exportServiceLogsFile(const QString &fileName)
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->exportLogsFile(fileName);
#else
    SystemController::saveFile(fileName, Logger::getServiceLogFile());
#endif
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

void SettingsController::backupAppConfig(const QString &fileName)
{
    SystemController::saveFile(fileName, m_settings->backupAppConfig());
}

void SettingsController::restoreAppConfig(const QString &fileName)
{
    QByteArray data;
    SystemController::readFile(fileName, data);
    restoreAppConfigFromData(data);
}

void SettingsController::restoreAppConfigFromData(const QByteArray &data)
{
    bool ok = m_settings->restoreAppConfig(data);
    if (ok) {
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
    m_settings->clearSettings();
    m_serversModel->resetModel();
    m_languageModel->changeLanguage(
            static_cast<LanguageSettings::AvailableLanguageEnum>(m_languageModel->getCurrentLanguageIndex()));

    m_sitesModel->setRouteMode(Settings::RouteMode::VpnOnlyForwardSites);
    m_sitesModel->toggleSplitTunneling(false);

    m_appSplitTunnelingModel->setRouteMode(Settings::AppsRouteMode::VpnAllExceptApps);
    m_appSplitTunnelingModel->toggleSplitTunneling(false);

    emit changeSettingsFinished(tr("All settings have been reset to default values"));

#ifdef Q_OS_IOS
    AmneziaVPN::clearSettings();
#endif
}

bool SettingsController::isAutoConnectEnabled()
{
    return m_settings->isAutoConnect();
}

void SettingsController::toggleAutoConnect(bool enable)
{
    m_settings->setAutoConnect(enable);
}

bool SettingsController::isAutoStartEnabled()
{
    return Autostart::isAutostart();
}

void SettingsController::toggleAutoStart(bool enable)
{
    Autostart::setAutostart(enable);
}

bool SettingsController::isStartMinimizedEnabled()
{
    return m_settings->isStartMinimized();
}

void SettingsController::toggleStartMinimized(bool enable)
{
    m_settings->setStartMinimized(enable);
}

bool SettingsController::isScreenshotsEnabled()
{
    return m_settings->isScreenshotsEnabled();
}

void SettingsController::toggleScreenshotsEnabled(bool enable)
{
    m_settings->setScreenshotsEnabled(enable);
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
    if (m_settings->isSaveLogs()) {
        m_loggingDisableDate = m_settings->getLogEnableDate().addDays(14);
        if (m_loggingDisableDate <= QDateTime::currentDateTime()) {
            toggleLogging(false);
            clearLogs();
            emit loggingDisableByWatcher();
        }
    }
}

bool SettingsController::isKillSwitchEnabled()
{
    return m_settings->isKillSwitchEnabled();
}

void SettingsController::toggleKillSwitch(bool enable)
{
    m_settings->setKillSwitchEnabled(enable);
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
    return m_settings->getInstallationUuid(false);
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
    m_settings->resetGatewayEndpoint();
    emit gatewayEndpointChanged(m_settings->getGatewayEndpoint());
}

void SettingsController::setGatewayEndpoint(const QString &endpoint)
{
    m_settings->setGatewayEndpoint(endpoint);
    emit gatewayEndpointChanged(endpoint);
}

QString SettingsController::getGatewayEndpoint()
{
    return m_settings->isDevGatewayEnv() ? "Dev endpoint" : m_settings->getGatewayEndpoint();
}

bool SettingsController::isDevGatewayEnv()
{
    return m_settings->isDevGatewayEnv();
}

void SettingsController::toggleDevGatewayEnv(bool enabled)
{
    m_settings->toggleDevGatewayEnv(enabled);
    if (enabled) {
        m_settings->setDevGatewayEndpoint();
    } else {
        m_settings->resetGatewayEndpoint();
    }
    emit gatewayEndpointChanged(m_settings->getGatewayEndpoint());
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
    return m_settings->isHomeAdLabelVisible();
}

void SettingsController::disableHomeAdLabel()
{
    m_settings->disableHomeAdLabel();
    emit isHomeAdLabelVisibleChanged(false);
}
