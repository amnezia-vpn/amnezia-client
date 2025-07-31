#include "settingsController.h"

#include <QDateTime>

#include "settings.h"
#include "logger.h"
#include "ui/qautostart.h"
#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

namespace
{
    Logger logger("SettingsController");
}

SettingsController::SettingsController(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

void SettingsController::resetAllSettings()
{
    logger.info() << "Resetting all settings to defaults";
    m_settings->clearSettings();
    emit settingsReset();
}

void SettingsController::configureDns(const QString &primaryDns, const QString &secondaryDns)
{
    m_settings->setPrimaryDns(primaryDns);
    m_settings->setSecondaryDns(secondaryDns);
    emit dnsConfigChanged();
}

void SettingsController::toggleAmneziaDns(bool enable)
{
    m_settings->setUseAmneziaDns(enable);
    emit dnsConfigChanged();
}

void SettingsController::configureLogging(bool enabled)
{
    m_settings->setIsLoggingEnabled(enabled);
}

void SettingsController::checkLoggingExpiration()
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

void SettingsController::clearLogs()
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

void SettingsController::configureKillSwitch(bool enable, bool strict)
{
    m_settings->setKillSwitchEnabled(enable);
    if (enable) {
        m_settings->setStrictKillSwitchEnabled(strict);
    } else {
        m_settings->setStrictKillSwitchEnabled(false);
    }
    emit killSwitchConfigChanged();
}

void SettingsController::configureAutoStart(bool enable)
{
    Autostart::setAutostart(enable);
    emit autoStartConfigChanged();
}

void SettingsController::configureAutoConnect(bool enable)
{
    m_settings->setAutoConnect(enable);
}

void SettingsController::configureStartMinimized(bool enable)
{
    m_settings->setStartMinimized(enable);
}

void SettingsController::configureScreenshots(bool enable)
{
    m_settings->setScreenshotsEnabled(enable);
}

QString SettingsController::getPrimaryDns() const
{
    return m_settings->primaryDns();
}

QString SettingsController::getSecondaryDns() const
{
    return m_settings->secondaryDns();
}

bool SettingsController::isAmneziaDnsEnabled() const
{
    return m_settings->isUseAmneziaDns();
}

bool SettingsController::isLoggingEnabled() const
{
    return m_settings->isSaveLogs();
}

bool SettingsController::isKillSwitchEnabled() const
{
    return m_settings->isKillSwitchEnabled();
}

bool SettingsController::isStrictKillSwitchEnabled() const
{
    return m_settings->isStrictKillSwitchEnabled();
}

bool SettingsController::isAutoStartEnabled() const
{
    return m_settings->isAutoStart();
}

bool SettingsController::isAutoConnectEnabled() const
{
    return m_settings->isAutoConnect();
}

bool SettingsController::isStartMinimizedEnabled() const
{
    return m_settings->isStartMinimized();
}

bool SettingsController::isScreenshotsEnabled() const
{
    return m_settings->isScreenshotsEnabled();
}

QByteArray SettingsController::backupAppConfig() const
{
    return m_settings->backupAppConfig();
}

bool SettingsController::restoreAppConfig(const QByteArray &data)
{
    return m_settings->restoreAppConfig(data);
}

QString SettingsController::getInstallationUuid() const
{
    return m_settings->getInstallationUuid(false);
}

void SettingsController::resetGatewayEndpoint()
{
    m_settings->resetGatewayEndpoint();
}

void SettingsController::setGatewayEndpoint(const QString &endpoint)
{
    m_settings->setGatewayEndpoint(endpoint);
}

QString SettingsController::getGatewayEndpoint() const
{
    if (m_settings->isDevGatewayEnv()) {
        return "Dev endpoint";
    }
    return m_settings->getGatewayEndpoint();
}

bool SettingsController::isDevGatewayEnv() const
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
}

void SettingsController::setDevGatewayEndpoint()
{
    m_settings->setDevGatewayEndpoint();
}

bool SettingsController::isHomeAdLabelVisible() const
{
    return m_settings->isHomeAdLabelVisible();
}

void SettingsController::disableHomeAdLabel()
{
    m_settings->disableHomeAdLabel();
}

QDateTime SettingsController::getLogEnableDate() const
{
    return m_settings->getLogEnableDate();
}

QLocale SettingsController::getAppLanguage() const
{
    return m_settings->getAppLanguage();
}

void SettingsController::setAppLanguage(const QLocale &locale)
{
    m_settings->setAppLanguage(locale);
}


