#ifndef UPDATECONTROLLER_H
#define UPDATECONTROLLER_H

#include <functional>
#include <QObject>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"

class QTimer;

class UpdateController : public QObject
{
    Q_OBJECT
public:
    explicit UpdateController(SecureAppSettingsRepository* appSettingsRepository,
                              SecureServersRepository* serversRepository,
                              QObject *parent = nullptr);

    QString getRawChangelogText() const;
    QString getReleaseDate() const;
    QString getVersion() const;

public slots:
    void checkForUpdates();
    void runInstaller();

signals:
    void updateFound();

private:
    struct UpdateArtifact
    {
        QString platform;
        QUrl url;
        QString sha256;
        qint64 size = -1;
        bool openExternally = false;
        bool autoInstall = false;
    };

    enum class InstallerHandoffResult
    {
        Failed,
        Started,
        PendingPermission
    };

    void finishUpdateCheck();
    void fetchSelfHostedManifest();
    void fetchSelfHostedManifestFromUrls(const QList<QUrl> &manifestUrls, int urlIndex);
    void fetchGatewayUrl();
    void fetchVersionInfo();
    void fetchChangelog();
    void fetchReleaseDate();
    void doGetAsync(const QString &endpoint, std::function<void(bool, QByteArray)> onDone);
    bool isSelfHostedUpdateChannelConfigured() const;
    bool isNewVersionAvailable() const;
    bool isNewVersionAvailable(const QString &version) const;
    QList<QUrl> selfHostedManifestUrls() const;
    QUrl normalizedSelfHostedManifestUrl(const QString &host) const;
    QList<QString> platformCandidates() const;
    bool processSelfHostedManifest(const QUrl &manifestUrl, const QByteArray &manifestData);
    bool verifySignedManifestEnvelope(const QByteArray &manifestData, QByteArray &payloadData) const;
    bool verifyManifestSignature(const QByteArray &payloadData, const QByteArray &signature) const;
    bool selectSelfHostedArtifact(const QUrl &manifestUrl, const QJsonObject &payload, UpdateArtifact &artifactOut);
    QUrl resolvedArtifactUrl(const QUrl &manifestUrl, const QString &urlOrPath) const;
    void startArtifactDownload();
    InstallerHandoffResult launchDownloadedArtifact(const QString &localPath);
    bool openArtifactExternally();
    bool shouldAutoInstallSelfHostedArtifact() const;
    QString selfHostedAutoInstallAttemptId() const;
    QString selfHostedAutoInstallAttemptMarker() const;
    void scheduleSelfHostedAutoInstall();
    void commitPendingAutoInstallAttempt();
    void clearPendingAutoInstallAttempt();
    void finishSelfHostedInstallerAttempt(InstallerHandoffResult result);
    void onAndroidApkInstallerStarted(const QString &fileName);
    void scheduleDesktopQuitAfterInstallerStart();
    void setupNetworkErrorHandling(QNetworkReply* reply, const QString& operation);
    void handleNetworkError(QNetworkReply* reply, const QString& operation);
    QString composeDownloadUrl() const;
    QString localInstallerPath() const;
    void startBackgroundUpdateChecks();

    SecureAppSettingsRepository* m_appSettingsRepository;
    SecureServersRepository* m_serversRepository;
    QTimer* m_backgroundUpdateTimer = nullptr;

    QString m_baseUrl;
    QString m_changelogText;
    QString m_version;
    QString m_releaseDate;
    QString m_downloadUrl;
    UpdateArtifact m_selectedArtifact;
    QString m_pendingAutoInstallAttemptId;
    bool m_useSelfHostedArtifact = false;
    bool m_updateCheckRunning = false;
    bool m_selfHostedInstallInProgress = false;
    bool m_androidApkInstallPermissionPending = false;

#if defined(Q_OS_WINDOWS)
    int runWindowsInstaller(const QString &installerPath);
#elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
    int runMacInstaller(const QString &installerPath);
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int runLinuxInstaller(const QString &installerPath);
#endif
};

#endif // UPDATECONTROLLER_H
