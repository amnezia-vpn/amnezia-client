#include "settingsConfigController.h"

#include <QDateTime>

#include "settings.h"
#include "logger.h"
#include "ui/qautostart.h"

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
    // TODO: Implement log clearing logic
    logger.info() << "Clearing logs";
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
    return m_settings->useAmneziaDns();
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
    return Autostart::isAutostart();
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
