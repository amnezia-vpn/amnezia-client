#include "settingsController.h"

#include <QStandardPaths>

#include "logger.h"
#include "utilities.h"
#include "version.h"

SettingsController::SettingsController(const QSharedPointer<ServersModel> &serversModel,
                                       const QSharedPointer<ContainersModel> &containersModel,
                                       const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel), m_settings(settings)
{
    m_appVersion = QString("%1: %2 (%3)").arg(tr("Software version"), QString(APP_MAJOR_VERSION), __DATE__);
}

void SettingsController::setAmneziaDns(bool enable)
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

bool SettingsController::isLoggingEnable()
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

void SettingsController::exportLogsFile()
{
    Utils::saveFile(".log", tr("Save log"), "AmneziaVPN", Logger::getLogFile());
}

void SettingsController::clearLogs()
{
    Logger::clearLogs();
    Logger::clearServiceLogs();
}

void SettingsController::backupAppConfig()
{
    Utils::saveFile(".backup", tr("Backup application config"), "AmneziaVPN", m_settings->backupAppConfig());
}

void SettingsController::restoreAppConfig()
{
    QString fileName =
            Utils::getFileName(Q_NULLPTR, tr("Open backup"),
                               QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.backup");

    // todo error processing
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();

    bool ok = m_settings->restoreAppConfig(data);
    if (ok) {
        //        emit uiLogic()->showWarningMessage(tr("Can't import config, file is corrupted."));
    }
}

QString SettingsController::getAppVersion()
{
    return m_appVersion;
}

void SettingsController::clearSettings()
{
    m_settings->clearSettings();
}
