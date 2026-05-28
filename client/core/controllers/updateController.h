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
    void runInstaller();

signals:
    void updateFound();

private:
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

#if defined(Q_OS_WINDOWS)
    int runWindowsInstaller(const QString &installerPath);
#elif defined(Q_OS_MACOS)
    int runMacInstaller(const QString &installerPath);
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int runLinuxInstaller(const QString &installerPath);
#endif
};

#endif // UPDATECONTROLLER_H
