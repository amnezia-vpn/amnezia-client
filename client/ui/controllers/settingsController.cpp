#include "settingsController.h"

#include <QStandardPaths>

#include "logger.h"
#include "systemController.h"
#include "ui/qautostart.h"
#include "version.h"

SettingsController::SettingsController(const QSharedPointer<ServersModel> &serversModel,
                                       const QSharedPointer<ContainersModel> &containersModel,
                                       const QSharedPointer<LanguageModel> &languageModel,
                                       const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_languageModel(languageModel),
      m_settings(settings)
{
    m_appVersion = QString("%1: %2 (%3)").arg(tr("Software version"), QString(APP_MAJOR_VERSION), __DATE__);
}

void SettingsController::toggleAmneziaDns(bool enable)
{
    m_settings->setUseAmneziaDns(enable);
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
    emit loggingStateChanged();
}

void SettingsController::openLogsFolder()
{
    Logger::openLogsFolder();
}

void SettingsController::exportLogsFile(const QString &fileName)
{
    SystemController::saveFile(fileName, Logger::getLogFile());
}

void SettingsController::clearLogs()
{
    Logger::clearLogs();
    Logger::clearServiceLogs();
}

void SettingsController::backupAppConfig(const QString &fileName)
{
    SystemController::saveFile(fileName, m_settings->backupAppConfig());
}

void SettingsController::restoreAppConfig(const QString &fileName)
{
    QFile file(fileName);

    file.open(QIODevice::ReadOnly);

    QByteArray data = file.readAll();

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
    emit changeSettingsFinished(tr("All settings have been reset to default values"));
}

void SettingsController::clearCachedProfiles()
{
    m_containersModel->clearCachedProfiles();
    emit changeSettingsFinished(tr("Cached profiles cleared"));
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
