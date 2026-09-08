#ifndef UPDATECONTROLLER_H
#define UPDATECONTROLLER_H

#include <QObject>
#include <QQmlEngine>
#include <QStringList>

#include "core/repositories/secureAppSettingsRepository.h"

namespace UpdateState
{
    Q_NAMESPACE
    enum class State {
        Idle = 0,
        Downloading,
        ReadyToInstall,
        DownloadError
    };
    Q_ENUM_NS(State)

    inline void declareQmlUpdateStateEnum()
    {
        qmlRegisterUncreatableMetaObject(UpdateState::staticMetaObject, "UpdateState", 1, 0, "UpdateState",
                                         "Error: only enums");
    }
}

class UpdateController : public QObject
{
    Q_OBJECT
public:
    explicit UpdateController(SecureAppSettingsRepository* appSettingsRepository, QObject *parent = nullptr);

    QString getVersion() const;
    QString getReleaseDate() const;
    QString getDescription() const;
    QStringList getTags() const;
    QStringList getNewFeatures() const;
    QStringList getImprovements() const;
    QStringList getBugFixes() const;
    UpdateState::State getUpdateState() const;
    bool isStoreUpdate() const;
    bool isUpdateCheckRunning() const;

public slots:
    void checkForUpdates();
    void startUpdate();
    void installUpdate();

signals:
    void updateFound();
    void updateNotFound();
    void updateCheckFailed();
    void updateStateChanged();
    void updateCheckRunningChanged();

private:
    void setUpdateState(UpdateState::State state);
    void setUpdateCheckRunning(bool running);
    void downloadInstaller();
    QString composeDownloadUrl() const;
    void openStorePage() const;

    SecureAppSettingsRepository* m_appSettingsRepository;

    QString m_version;
    QString m_releaseDate;
    QString m_description;
    QStringList m_tags;
    QStringList m_newFeatures;
    QStringList m_improvements;
    QStringList m_bugFixes;
    QString m_downloadBaseUrl;
    QString m_releasePageUrl;

    UpdateState::State m_updateState = UpdateState::State::Idle;
    bool m_updateCheckRunning = false;

#if defined(Q_OS_WINDOWS)
    int runWindowsInstaller(const QString &installerPath);
#elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
    int runMacInstaller(const QString &installerPath);
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int runLinuxInstaller(const QString &installerPath);
#endif
};

#endif // UPDATECONTROLLER_H
