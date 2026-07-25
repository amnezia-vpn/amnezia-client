#ifndef UPDATECONTROLLER_H
#define UPDATECONTROLLER_H

#include <functional>
#include <QObject>
#include <QNetworkReply>

#include "core/repositories/secureAppSettingsRepository.h"

class UpdateController : public QObject
{
    Q_OBJECT
public:
    explicit UpdateController(SecureAppSettingsRepository* appSettingsRepository, QObject *parent = nullptr);

    QString getRawChangelogText() const;
    QString getReleaseDate() const;
    QString getVersion() const;

public slots:
    void checkForUpdates();
    // Fetches version/changelog/release date without comparing versions and without
    // emitting updateFound(). Used on mobile, where the store is the update trigger
    // and the gateway only supplies the changelog text.
    void fetchReleaseInfo();
    void runInstaller();

signals:
    void updateFound();
    // Emitted once fetchReleaseInfo() has finished, regardless of success.
    void releaseInfoFetched();

private:
    enum class CheckMode {
        UpdateCheck,
        ReleaseInfoOnly
    };

    void finishUpdateCheck();
    void fetchGatewayUrl();
    void fetchVersionInfo();
    void fetchChangelog();
    void fetchReleaseDate();
    void doGetAsync(const QString &endpoint, std::function<void(bool, QByteArray)> onDone);
    bool isNewVersionAvailable() const;
    void setupNetworkErrorHandling(QNetworkReply* reply, const QString& operation);
    void handleNetworkError(QNetworkReply* reply, const QString& operation);
    QString composeDownloadUrl() const;

    SecureAppSettingsRepository* m_appSettingsRepository;

    QString m_baseUrl;
    QString m_changelogText;
    QString m_version;
    QString m_releaseDate;
    QString m_downloadUrl;
    bool m_updateCheckRunning = false;
    CheckMode m_checkMode = CheckMode::UpdateCheck;

#if defined(Q_OS_WINDOWS)
    int runWindowsInstaller(const QString &installerPath);
#elif defined(Q_OS_MACOS)
    int runMacInstaller(const QString &installerPath);
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int runLinuxInstaller(const QString &installerPath);
#endif
};

#endif // UPDATECONTROLLER_H
