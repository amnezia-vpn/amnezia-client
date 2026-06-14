#include "settingsUiController.h"

#include <QDebug>
#include <QStandardPaths>
#include <QOperatingSystemVersion>
#include <QFile>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "logger.h"
#include "systemController.h"
#include "amneziaApplication.h"
#include "version.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <AmneziaVPN-Swift.h>
#endif

SettingsUiController::SettingsUiController(SettingsController* settingsController,
                                         ServersController* serversController,
                                         QObject *parent)
    : QObject(parent),
      m_settingsController(settingsController),
      m_serversController(serversController)
{
#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::notificationStateChanged, this, &SettingsUiController::onNotificationStateChanged);
    connect(AndroidController::instance(), &AndroidController::activityPaused, this, &SettingsUiController::activityPaused);
    connect(AndroidController::instance(), &AndroidController::activityResumed, this, &SettingsUiController::activityResumed);
#endif

    m_settingsController->checkIfNeedDisableLogs();
    if (m_settingsController->isDevGatewayEnv()) {
        m_settingsController->enableDevMode();
    }
}

void SettingsUiController::toggleAmneziaDns(bool enable)
{
    m_settingsController->toggleAmneziaDns(enable);
    emit amneziaDnsToggled(enable);
}

bool SettingsUiController::isAmneziaDnsEnabled()
{
    return m_settingsController->isAmneziaDnsEnabled();
}

QString SettingsUiController::getPrimaryDns()
{
    return m_settingsController->getPrimaryDns();
}

void SettingsUiController::setPrimaryDns(const QString &dns)
{
    m_settingsController->setPrimaryDns(dns);
    emit primaryDnsChanged();
}

QString SettingsUiController::getSecondaryDns()
{
    return m_settingsController->getSecondaryDns();
}

void SettingsUiController::setSecondaryDns(const QString &dns)
{
    m_settingsController->setSecondaryDns(dns);
    emit secondaryDnsChanged();
}

bool SettingsUiController::isLoggingEnabled()
{
    return m_settingsController->isLoggingEnabled();
}

void SettingsUiController::toggleLogging(bool enable)
{
    m_settingsController->toggleLogging(enable);
#if defined(Q_OS_IOS)
    AmneziaVPN::toggleLogging(enable);
#endif
    if (enable == true) {
        qInfo().noquote() << QString("Logging has enabled on %1 version %2 %3").arg(APPLICATION_NAME, APP_VERSION, GIT_COMMIT_HASH);
        qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    }
    emit loggingStateChanged();
}

void SettingsUiController::openLogsFolder()
{
    Logger::openLogsFolder(false);
}

void SettingsUiController::openServiceLogsFolder()
{
    Logger::openLogsFolder(true);
}

void SettingsUiController::exportLogsFile(const QString &fileName)
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->exportLogsFile(fileName);
#else
    if (!SystemController::saveFile(fileName, Logger::getLogFile())) {
        qInfo() << "SettingsUiController::exportLogsFile: save or share was cancelled or failed";
    }
#endif
}

void SettingsUiController::exportServiceLogsFile(const QString &fileName)
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->exportLogsFile(fileName);
#else
    if (!SystemController::saveFile(fileName, Logger::getServiceLogFile())) {
        qInfo() << "SettingsUiController::exportServiceLogsFile: save or share was cancelled or failed";
    }
#endif
}

void SettingsUiController::clearLogs()
{
    m_settingsController->clearLogs();

    qInfo().noquote() << QString("Started %1 version %2 %3").arg(APPLICATION_NAME, APP_VERSION, GIT_COMMIT_HASH);
    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    qInfo().noquote() << QString("SSL backend: %1").arg(QSslSocket::sslLibraryVersionString());
}

void SettingsUiController::backupAppConfig(const QString &fileName)
{
    QByteArray data = m_settingsController->backupAppConfig();
    if (!SystemController::saveFile(fileName, data)) {
        qInfo() << "SettingsUiController::backupAppConfig: save or share was cancelled or failed";
    }
}

void SettingsUiController::restoreAppConfig(const QString &fileName)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(ErrorCode::OpenError);
        return;
    }

    restoreAppConfigFromData(file.readAll());
}

void SettingsUiController::restoreAppConfigFromData(const QByteArray &data)
{
    ErrorCode errorCode = m_settingsController->restoreAppConfigFromData(data);
    if (errorCode == ErrorCode::NoError) {
        emit appLanguageChanged();

        bool amneziaDnsEnabled = m_settingsController->isAmneziaDnsEnabled();
        emit amneziaDnsToggled(amneziaDnsEnabled);

        emit restoreBackupFinished();
        emit autoStartChanged();
        emit startMinimizedChanged();
    } else {
        emit errorOccurred(errorCode);
    }
}

QString SettingsUiController::getAppVersion()
{
    return m_settingsController->getAppVersion();
}

void SettingsUiController::clearSettings()
{
    m_settingsController->clearSettings();
    emit autoStartChanged();
    emit startMinimizedChanged();
    emit resetLanguageToSystem();

    emit changeSettingsFinished(tr("All settings have been reset to default values"));

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    AmneziaVPN::clearSettings();
#endif
}

bool SettingsUiController::isAutoConnectEnabled()
{
    return m_settingsController->isAutoConnectEnabled();
}

void SettingsUiController::toggleAutoConnect(bool enable)
{
    m_settingsController->toggleAutoConnect(enable);
}

bool SettingsUiController::isAutoStartEnabled()
{
    return m_settingsController->isAutoStartEnabled();
}

void SettingsUiController::toggleAutoStart(bool enable)
{
    m_settingsController->toggleAutoStart(enable);
    emit autoStartChanged();
    emit startMinimizedChanged();
}

bool SettingsUiController::isStartMinimizedEnabled()
{
    return m_settingsController->isStartMinimizedEnabled();
}

void SettingsUiController::toggleStartMinimized(bool enable)
{
    m_settingsController->toggleStartMinimized(enable);
    emit startMinimizedChanged();
}

bool SettingsUiController::isScreenshotsEnabled()
{
    return m_settingsController->isScreenshotsEnabled();
}

void SettingsUiController::toggleScreenshotsEnabled(bool enable)
{
    m_settingsController->toggleScreenshotsEnabled(enable);
}

bool SettingsUiController::isNewsNotificationsEnabled()
{
    return m_settingsController->isNewsNotificationsEnabled();
}

void SettingsUiController::toggleNewsNotificationsEnabled(bool enable)
{
    m_settingsController->toggleNewsNotificationsEnabled(enable);
}

bool SettingsUiController::isCameraPresent()
{
#if defined Q_OS_IOS
    return true;
#elif defined Q_OS_ANDROID
    return AndroidController::instance()->isCameraPresent();
#else
    return false;
#endif
}

bool SettingsUiController::isKillSwitchEnabled()
{
    return m_settingsController->isKillSwitchEnabled();
}

void SettingsUiController::toggleKillSwitch(bool enable)
{
    m_settingsController->toggleKillSwitch(enable);
    emit killSwitchEnabledChanged();
    if (enable == false) {
        emit strictKillSwitchEnabledChanged(false);
    } else {
        emit strictKillSwitchEnabledChanged(isStrictKillSwitchEnabled());
    }
}

bool SettingsUiController::isStrictKillSwitchEnabled()
{
    return m_settingsController->isStrictKillSwitchEnabled();
}

void SettingsUiController::toggleStrictKillSwitch(bool enable)
{
    m_settingsController->toggleStrictKillSwitch(enable);
    emit strictKillSwitchEnabledChanged(enable);
}

bool SettingsUiController::isNotificationPermissionGranted()
{
#ifdef Q_OS_ANDROID
    return AndroidController::instance()->isNotificationPermissionGranted();
#else
    return true;
#endif
}

void SettingsUiController::requestNotificationPermission()
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->requestNotificationPermission();
#endif
}

QString SettingsUiController::getInstallationUuid()
{
    return m_settingsController->getInstallationUuid();
}

void SettingsUiController::enableDevMode()
{
    m_settingsController->enableDevMode();
    emit devModeEnabled();
}

bool SettingsUiController::isDevModeEnabled()
{
    return m_settingsController->isDevModeEnabled();
}

void SettingsUiController::resetGatewayEndpoint()
{
    m_settingsController->resetGatewayEndpoint();
    emit gatewayEndpointChanged(m_settingsController->getGatewayEndpoint());
}

void SettingsUiController::setGatewayEndpoint(const QString &endpoint)
{
    m_settingsController->setGatewayEndpoint(endpoint);
    emit gatewayEndpointChanged(endpoint);
}

QString SettingsUiController::getGatewayEndpoint()
{
    return m_settingsController->getGatewayEndpoint();
}

bool SettingsUiController::isDevGatewayEnv()
{
    return m_settingsController->isDevGatewayEnv();
}

void SettingsUiController::toggleDevGatewayEnv(bool enabled)
{
    m_settingsController->toggleDevGatewayEnv(enabled);
    emit gatewayEndpointChanged(m_settingsController->getGatewayEndpoint());
    emit devGatewayEnvChanged(enabled);
}

bool SettingsUiController::isOnTv()
{
#ifdef Q_OS_ANDROID
    return AndroidController::instance()->isOnTv();
#else
    return false;
#endif
}

bool SettingsUiController::isHomeAdLabelVisible()
{
    return m_settingsController->isHomeAdLabelVisible();
}

void SettingsUiController::disableHomeAdLabel()
{
    m_settingsController->disableHomeAdLabel();
    emit isHomeAdLabelVisibleChanged(false);
}
