#include "settingsConfigController.h"

#include <QDateTime>

#include "settings.h"
#include "logger.h"
#include "ui/qautostart.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

namespace
{
    Logger logger("SettingsConfigController");
}

SettingsConfigController::SettingsConfigController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

void SettingsConfigController::resetAllSettings()
{
    logger.info() << "Resetting all settings to defaults";
    m_settings->clearSettings();
    emit settingsReset();
}

void SettingsConfigController::configureDns(const QString &primaryDns, const QString &secondaryDns)
{
    m_settings->setPrimaryDns(primaryDns);
    m_settings->setSecondaryDns(secondaryDns);
    emit dnsConfigChanged();
}

void SettingsConfigController::toggleAmneziaDns(bool enable)
{
    m_settings->setUseAmneziaDns(enable);
    emit dnsConfigChanged();
}

void SettingsConfigController::configureLogging(bool enable)
{
    m_settings->setSaveLogs(enable);
    if (enable) {
        m_settings->setLogEnableDate(QDateTime::currentDateTime());
    }
    emit loggingConfigChanged();
}

void SettingsConfigController::checkLoggingExpiration()
{
    if (m_settings->isSaveLogs()) {
        QDateTime loggingDisableDate = m_settings->getLogEnableDate().addDays(14);
        if (loggingDisableDate <= QDateTime::currentDateTime()) {
            configureLogging(false);
            clearLogs();
            emit loggingExpired();
        }
    }
}

void SettingsConfigController::clearLogs()
{
    logger.info() << "Clearing application logs";
    
#ifdef Q_OS_ANDROID
    AndroidController::instance()->clearLogs();
#else
    Logger::clearLogs(false);
    Logger::clearServiceLogs();
#endif
    
    logger.info() << "Logs cleared successfully";
}

void SettingsConfigController::configureKillSwitch(bool enable, bool strict)
{
    m_settings->setKillSwitchEnabled(enable);
    if (enable) {
        m_settings->setStrictKillSwitchEnabled(strict);
    } else {
        m_settings->setStrictKillSwitchEnabled(false);
    }
    emit killSwitchConfigChanged();
}

void SettingsConfigController::configureAutoStart(bool enable)
{
    Autostart::setAutostart(enable);
    emit autoStartConfigChanged();
}

void SettingsConfigController::configureAutoConnect(bool enable)
{
    m_settings->setAutoConnect(enable);
}

void SettingsConfigController::configureStartMinimized(bool enable)
{
    m_settings->setStartMinimized(enable);
}

void SettingsConfigController::configureScreenshots(bool enable)
{
    m_settings->setScreenshotsEnabled(enable);
}

QString SettingsConfigController::getPrimaryDns() const
{
    return m_settings->primaryDns();
}

QString SettingsConfigController::getSecondaryDns() const
{
    return m_settings->secondaryDns();
}

bool SettingsConfigController::isAmneziaDnsEnabled() const
{
    return m_settings->isUseAmneziaDns();
}

bool SettingsConfigController::isLoggingEnabled() const
{
    return m_settings->isSaveLogs();
}

bool SettingsConfigController::isKillSwitchEnabled() const
{
    return m_settings->isKillSwitchEnabled();
}

bool SettingsConfigController::isStrictKillSwitchEnabled() const
{
    return m_settings->isStrictKillSwitchEnabled();
}

bool SettingsConfigController::isAutoStartEnabled() const
{
    return m_settings->isAutoStart();
}

bool SettingsConfigController::isAutoConnectEnabled() const
{
    return m_settings->isAutoConnect();
}

bool SettingsConfigController::isStartMinimizedEnabled() const
{
    return m_settings->isStartMinimized();
}

bool SettingsConfigController::isScreenshotsEnabled() const
{
    return m_settings->isScreenshotsEnabled();
}

QByteArray SettingsConfigController::backupAppConfig() const
{
    return m_settings->backupAppConfig();
}

bool SettingsConfigController::restoreAppConfig(const QByteArray &data)
{
    return m_settings->restoreAppConfig(data);
}

QString SettingsConfigController::getInstallationUuid() const
{
    return m_settings->getInstallationUuid(false);
}

void SettingsConfigController::resetGatewayEndpoint()
{
    m_settings->resetGatewayEndpoint();
}

void SettingsConfigController::setGatewayEndpoint(const QString &endpoint)
{
    m_settings->setGatewayEndpoint(endpoint);
}

QString SettingsConfigController::getGatewayEndpoint() const
{
    if (m_settings->isDevGatewayEnv()) {
        return "Dev endpoint";
    }
    return m_settings->getGatewayEndpoint();
}

bool SettingsConfigController::isDevGatewayEnv() const
{
    return m_settings->isDevGatewayEnv();
}

void SettingsConfigController::toggleDevGatewayEnv(bool enabled)
{
    m_settings->toggleDevGatewayEnv(enabled);
    if (enabled) {
        m_settings->setDevGatewayEndpoint();
    } else {
        m_settings->resetGatewayEndpoint();
    }
}

void SettingsConfigController::setDevGatewayEndpoint()
{
    m_settings->setDevGatewayEndpoint();
}

bool SettingsConfigController::isHomeAdLabelVisible() const
{
    return m_settings->isHomeAdLabelVisible();
}

void SettingsConfigController::disableHomeAdLabel()
{
    m_settings->disableHomeAdLabel();
}

QDateTime SettingsConfigController::getLogEnableDate() const
{
    return m_settings->getLogEnableDate();
}


