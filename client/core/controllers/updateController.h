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
    void installerVerificationFailed(const QString &reason);

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
    QString m_expectedSha256;
    bool m_updateCheckRunning = false;

    // Verify the downloaded installer before launching it (fail-closed):
    // Windows/Linux compare a SHA-256 delivered over the encrypted gateway channel.
    bool verifySha256(const QByteArray &data) const;

#if defined(Q_OS_WINDOWS)
    int runWindowsInstaller(const QString &installerPath);
#elif defined(Q_OS_MACOS)
    // macOS verifies the .pkg Gatekeeper assessment (notarized + pinned Developer ID).
    bool verifyMacInstallerSignature(const QString &installerPath) const;
    int runMacInstaller(const QString &installerPath);
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int runLinuxInstaller(const QString &installerPath);
#endif
};

#endif // UPDATECONTROLLER_H
