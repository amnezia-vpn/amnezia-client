#include "settingsController.h"

#include <QStandardPaths>

#include "logger.h"
#include "systemController.h"
#include "ui/qautostart.h"
#include "version.h"
#ifdef Q_OS_ANDROID
    #include "../../platforms/android/android_controller.h"
    #include "../../platforms/android/androidutils.h"
    #include <QJniObject>
#endif

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
    m_appVersion = QString("%1: %2 (%3)").arg(tr("Software version"), QString(APP_VERSION), __DATE__);

#ifdef Q_OS_ANDROID
    if (!m_settings->isScreenshotsEnabled()) {
        // Set security screen for Android app
        AndroidUtils::runOnAndroidThreadSync([]() {
            QJniObject activity = AndroidUtils::getActivity();
            QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
            if (window.isValid()) {
                const int FLAG_SECURE = 8192;
                window.callMethod<void>("addFlags", "(I)V", FLAG_SECURE);
            }
        });
    }
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
    m_serversModel->clearCachedProfiles();
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

bool SettingsController::isScreenshotsEnabled()
{
    return m_settings->isScreenshotsEnabled();
}

void SettingsController::toggleScreenshotsEnabled(bool enable)
{
    m_settings->setScreenshotsEnabled(enable);
#ifdef Q_OS_ANDROID
    std::string command = enable ? "clearFlags" : "addFlags";

    // Set security screen for Android app
    AndroidUtils::runOnAndroidThreadSync([&command]() {
        QJniObject activity = AndroidUtils::getActivity();
        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (window.isValid()) {
            const int FLAG_SECURE = 8192;
            window.callMethod<void>(command.c_str(), "(I)V", FLAG_SECURE);
        }
    });
#endif
}
