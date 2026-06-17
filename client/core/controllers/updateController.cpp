#include "updateController.h"

#include <QCryptographicHash>
#include <QDate>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QProcess>
#include <QStandardPaths>
#include <QVersionNumber>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTimer>

#include <openssl/evp.h>
#include <openssl/pem.h>

#include "amneziaApplication.h"
#include "logger.h"
#include "version.h"
#include "core/controllers/gatewayController.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#if defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

namespace
{
    Logger logger("UpdateController");

#if defined(Q_OS_WINDOWS)
    const QLatin1String kInstallerRemoteFileNamePattern("AmneziaVPN_%1_windows_x64.exe");
#elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
    const QLatin1String kInstallerRemoteFileNamePattern("AmneziaVPN_%1_macos_x64.pkg");
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    const QLatin1String kInstallerRemoteFileNamePattern("AmneziaVPN_%1_linux_x64.run");
#endif

#ifndef SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64
#define SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64 ""
#endif

    constexpr qsizetype kManifestMaxPayloadBytes = 1024 * 1024;
    constexpr int kManifestTransferTimeoutMs = 7000;
    constexpr int kInstallerTransferTimeoutMs = 30 * 60 * 1000;
    constexpr int kInitialBackgroundUpdateCheckMs = 60 * 1000;
    constexpr int kBackgroundUpdateCheckIntervalMs = 6 * 60 * 60 * 1000;
    constexpr int kDesktopQuitAfterInstallerStartMs = 1500;
    constexpr int kAndroidApkInstallPermissionWaitMs = 10 * 60 * 1000;
    constexpr int kAndroidApkInstallFailed = 0;
    constexpr int kAndroidApkInstallStarted = 1;
    constexpr int kAndroidApkInstallPermissionSettingsOpened = 2;

    QString selfHostedUpdateUrl(const QString &host, const QString &path)
    {
        const QString trimmedHost = host.trimmed().isEmpty()
                ? QString::fromLatin1(amnezia::protocols::selfHostedUpdates::syncHost)
                : host.trimmed();
        const QString normalizedPath = path.startsWith('/') ? path : QStringLiteral("/%1").arg(path);

        QString endpoint = trimmedHost;
        if (!endpoint.contains(QStringLiteral("://"))) {
            const bool looksLikeUnbracketedIpv6 = endpoint.count(QLatin1Char(':')) > 1
                    && !endpoint.startsWith(QLatin1Char('['));
            endpoint = looksLikeUnbracketedIpv6
                    ? QStringLiteral("http://[%1]").arg(endpoint)
                    : QStringLiteral("http://%1").arg(endpoint);
        }

        QUrl url(endpoint);
        if (url.port() < 0) {
            url.setPort(amnezia::protocols::selfHostedUpdates::syncPort);
        }
        url.setPath(normalizedPath);
        url.setQuery(QString());
        url.setFragment(QString());
        return url.toString();
    }

    QString normalizedHexSha256(const QString &value)
    {
        QString normalized = value.trimmed().toLower();
        normalized.remove(QLatin1Char(':'));
        return normalized;
    }

    bool isSha256Hex(const QString &value)
    {
        if (value.size() != 64) {
            return false;
        }
        for (const QChar &ch : value) {
            if (!ch.isDigit() && (ch < QLatin1Char('a') || ch > QLatin1Char('f'))) {
                return false;
            }
        }
        return true;
    }

    bool isHttpOrHttpsUrl(const QUrl &url)
    {
        const QString scheme = url.scheme().toLower();
        return (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) && !url.host().isEmpty();
    }

    bool isAllowedExternalUpdateUrl(const QUrl &url)
    {
        if (!url.isValid() || url.isEmpty()) {
            return false;
        }

        const QString scheme = url.scheme().toLower();
#if defined(Q_OS_IOS)
        if (scheme == QStringLiteral("http")) {
            return false;
        }
#endif
        if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) {
            return !url.host().isEmpty();
        }
#if defined(Q_OS_IOS)
        if (scheme == QStringLiteral("itms-services")) {
            const QUrlQuery query(url);
            const QUrl manifestUrl(query.queryItemValue(QStringLiteral("url")));
            return manifestUrl.scheme().toLower() == QStringLiteral("https") && !manifestUrl.host().isEmpty();
        }
        if (scheme == QStringLiteral("itms-apps")) {
            return !url.host().isEmpty();
        }
        return false;
#else
        return false;
#endif
    }

    bool decodeStrictBase64(const QByteArray &encoded, QByteArray::Base64Options options, QByteArray &decoded)
    {
        const QByteArray::FromBase64Result result = QByteArray::fromBase64Encoding(
                encoded, options | QByteArray::AbortOnBase64DecodingErrors);
        if (!result) {
            decoded.clear();
            return false;
        }

        decoded = result.decoded;
        return true;
    }
}

UpdateController::UpdateController(SecureAppSettingsRepository* appSettingsRepository,
                                   SecureServersRepository* serversRepository,
                                   QObject *parent)
    : QObject(parent), m_appSettingsRepository(appSettingsRepository), m_serversRepository(serversRepository)
{
#if defined(Q_OS_ANDROID)
    connect(AndroidController::instance(), &AndroidController::apkInstallerStarted,
            this, &UpdateController::onAndroidApkInstallerStarted);
#endif
    startBackgroundUpdateChecks();
}

QString UpdateController::getRawChangelogText() const
{
    return m_changelogText;
}

QString UpdateController::getReleaseDate() const
{
    return m_releaseDate;
}

QString UpdateController::getVersion() const
{
    return m_version;
}

bool UpdateController::isUpdateCheckRunning() const
{
    return m_updateCheckRunning;
}

bool UpdateController::checkForUpdates()
{
    if (m_updateCheckRunning || m_selfHostedInstallInProgress || m_androidApkInstallPermissionPending || !m_appSettingsRepository) {
        return false;
    }
    m_updateCheckRunning = true;
    m_updateFoundDuringCheck = false;
    m_useSelfHostedArtifact = false;
    m_selectedArtifact = {};
    m_pendingAutoInstallAttemptId.clear();
    m_downloadUrl.clear();

    if (isSelfHostedUpdateChannelConfigured()) {
        fetchSelfHostedManifest();
    } else {
        fetchGatewayUrl();
    }
    return true;
}

void UpdateController::finishUpdateCheck()
{
    const bool updateAvailable = m_updateFoundDuringCheck;
    m_updateCheckRunning = false;
    emit updateCheckFinished(updateAvailable);
}

void UpdateController::startBackgroundUpdateChecks()
{
    m_backgroundUpdateTimer = new QTimer(this);
    m_backgroundUpdateTimer->setInterval(kBackgroundUpdateCheckIntervalMs);
    m_backgroundUpdateTimer->setSingleShot(false);
    connect(m_backgroundUpdateTimer, &QTimer::timeout, this, &UpdateController::checkForUpdates);
    m_backgroundUpdateTimer->start();

    QTimer::singleShot(kInitialBackgroundUpdateCheckMs, this, &UpdateController::checkForUpdates);
}

void UpdateController::fetchSelfHostedManifest()
{
    const QList<QUrl> manifestUrls = selfHostedManifestUrls();
    if (manifestUrls.isEmpty()) {
        finishUpdateCheck();
        return;
    }

    fetchSelfHostedManifestFromUrls(manifestUrls, 0);
}

void UpdateController::fetchSelfHostedManifestFromUrls(const QList<QUrl> &manifestUrls, int urlIndex)
{
    if (urlIndex < 0 || urlIndex >= manifestUrls.size()) {
        finishUpdateCheck();
        return;
    }

    const QUrl manifestUrl = manifestUrls.at(urlIndex);
    QNetworkRequest request(manifestUrl);
    request.setTransferTimeout(kManifestTransferTimeoutMs);

    QNetworkReply *reply = amnApp->networkManager()->get(request);
    auto *manifestData = new QByteArray();
    auto *manifestTooLarge = new bool(false);
    QObject::connect(reply, &QIODevice::readyRead, this, [reply, manifestData, manifestTooLarge]() {
        if (manifestData->size() > kManifestMaxPayloadBytes) {
            return;
        }
        manifestData->append(reply->readAll());
        if (manifestData->size() > kManifestMaxPayloadBytes) {
            *manifestTooLarge = true;
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::metaDataChanged, this, [reply, manifestTooLarge]() {
        const QVariant contentLength = reply->header(QNetworkRequest::ContentLengthHeader);
        if (contentLength.isValid() && contentLength.toLongLong() > kManifestMaxPayloadBytes) {
            *manifestTooLarge = true;
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, this,
                     [this, reply, manifestData, manifestTooLarge, manifestUrls, urlIndex, manifestUrl]() {
        const bool ok = (reply->error() == QNetworkReply::NoError);
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString errorString = ok ? QString() : reply->errorString();
        const QByteArray data = ok ? *manifestData : QByteArray();
        reply->deleteLater();

        if (!ok || statusCode < 200 || statusCode >= 300 || *manifestTooLarge || data.size() > kManifestMaxPayloadBytes
            || !processSelfHostedManifest(manifestUrl, data)) {
            if (!ok) {
                logger.info() << "Self-hosted update manifest unavailable at" << manifestUrl.toString() << errorString;
            }
            delete manifestData;
            delete manifestTooLarge;
            fetchSelfHostedManifestFromUrls(manifestUrls, urlIndex + 1);
            return;
        }

        delete manifestData;
        delete manifestTooLarge;
        m_updateFoundDuringCheck = true;
        emit updateFound();
        scheduleSelfHostedAutoInstall();
        finishUpdateCheck();
    });
}

void UpdateController::doGetAsync(const QString &endpoint, std::function<void(bool, QByteArray)> onDone)
{
    QString fullUrl = m_baseUrl + endpoint;

    QNetworkRequest req;
    req.setTransferTimeout(7000);
    req.setUrl(QUrl(fullUrl));

    QNetworkReply *reply = amnApp->networkManager()->get(req);
    setupNetworkErrorHandling(reply, endpoint);

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, endpoint, onDone]() {
        const bool ok = (reply->error() == QNetworkReply::NoError);
        QByteArray data;
        if (ok) {
            data = reply->readAll();
        } else {
            handleNetworkError(reply, endpoint);
        }
        reply->deleteLater();
        onDone(ok, data);
    });
}

void UpdateController::fetchGatewayUrl()
{
    auto gatewayController = QSharedPointer<GatewayController>::create(m_appSettingsRepository->getGatewayEndpoint(),
                                                                       m_appSettingsRepository->isDevGatewayEnv(),
                                                                       7000,
                                                                       m_appSettingsRepository->isStrictKillSwitchEnabled(),
                                                                       m_appSettingsRepository);

    QJsonObject apiPayload;
    apiPayload[apiDefs::key::cliVersion] = QString(APP_VERSION);
    apiPayload[apiDefs::key::osVersion] = QSysInfo::productType();
    apiPayload[apiDefs::key::installationUuid] = m_appSettingsRepository->getInstallationUuid(true);

    // Workaround: wait before contacting gateway to avoid rate limit triggered by other requests (news etc.)
    QTimer::singleShot(1000, this, [this, gatewayController, apiPayload]() {
        gatewayController->postAsync(QStringLiteral("%1v1/updater_endpoint"), apiPayload)
            .then(this, [this, gatewayController](QPair<ErrorCode, QByteArray> result) {
                auto [err, gatewayResponse] = result;
                if (err != ErrorCode::NoError) {
                    logger.error() << "Gateway request failed, error code:" << static_cast<int>(err);
                    finishUpdateCheck();
                    return;
                }

                QJsonObject gatewayData = QJsonDocument::fromJson(gatewayResponse).object();

                QString baseUrl = gatewayData.value("url").toString();
                if (baseUrl.endsWith('/')) {
                    baseUrl.chop(1);
                }
                m_baseUrl = baseUrl;

                fetchVersionInfo();
            });
    });
}

void UpdateController::fetchVersionInfo()
{
    doGetAsync("/VERSION", [this](bool ok, QByteArray data) {
        if (!ok) {
            finishUpdateCheck();
            return;
        }
        m_version = QString::fromUtf8(data).trimmed();

        if (!isNewVersionAvailable()) {
            finishUpdateCheck();
            return;
        }
        fetchChangelog();
    });
}

void UpdateController::fetchChangelog()
{
    doGetAsync("/CHANGELOG", [this](bool ok, QByteArray data) {
        if (!ok) {
            m_changelogText.clear();
        } else {
            m_changelogText = QString::fromUtf8(data);
        }
        fetchReleaseDate();
    });
}

void UpdateController::fetchReleaseDate()
{
    doGetAsync("/RELEASE_DATE", [this](bool ok, QByteArray data) {
        if (ok) {
            m_releaseDate = QString::fromUtf8(data).trimmed();
        } else {
            m_releaseDate = QString();
        }

        m_downloadUrl = composeDownloadUrl();
        if (m_downloadUrl.isEmpty()) {
            logger.info() << "Update is available on gateway, but this platform has no installer URL";
            finishUpdateCheck();
            return;
        }
        m_updateFoundDuringCheck = true;
        emit updateFound();
        finishUpdateCheck();
    });
}

bool UpdateController::isNewVersionAvailable() const
{
    return isNewVersionAvailable(m_version);
}

bool UpdateController::isSelfHostedUpdateChannelConfigured() const
{
    return !QByteArray(SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64).trimmed().isEmpty();
}

bool UpdateController::isNewVersionAvailable(const QString &version) const
{
    const auto currentVersion = QVersionNumber::fromString(QString(APP_VERSION));
    const auto newVersion = QVersionNumber::fromString(version);
    return newVersion > currentVersion;
}

QList<QUrl> UpdateController::selfHostedManifestUrls() const
{
    QList<QUrl> urls;
    const auto addHost = [this, &urls](const QString &host) {
        const QUrl url = normalizedSelfHostedManifestUrl(host);
        if (!url.isValid() || url.isEmpty()) {
            return;
        }
        const QString normalized = url.toString(QUrl::FullyEncoded);
        for (const QUrl &existing : urls) {
            if (existing.toString(QUrl::FullyEncoded) == normalized) {
                return;
            }
        }
        urls.append(url);
    };

    QStringList serverCredentialHosts;
    if (m_serversRepository) {
        const QString defaultServerId = m_serversRepository->defaultServerId();
        const QVector<QString> orderedServerIds = m_serversRepository->orderedServerIds();
        QStringList serverIds;
        if (!defaultServerId.isEmpty()) {
            serverIds.append(defaultServerId);
        }
        for (const QString &serverId : orderedServerIds) {
            if (!serverIds.contains(serverId)) {
                serverIds.append(serverId);
            }
        }
        for (const QString &serverId : serverIds) {
            const int serverIndex = m_serversRepository->indexOfServerId(serverId);
            if (serverIndex < 0) {
                continue;
            }
            const QJsonObject serverJson = m_serversRepository->serverJson(serverIndex);
            addHost(serverJson.value(configKey::serverRoutingRulesSyncHost).toString());
            const ServerCredentials credentials = m_serversRepository->serverCredentials(serverIndex);
            serverCredentialHosts.append(credentials.hostName);
        }
    }
    addHost(QString::fromLatin1(amnezia::protocols::selfHostedUpdates::syncHost));
    for (const QString &host : serverCredentialHosts) {
        addHost(host);
    }

    return urls;
}

QUrl UpdateController::normalizedSelfHostedManifestUrl(const QString &host) const
{
    const QString trimmedHost = host.trimmed();
    if (trimmedHost.isEmpty()) {
        return {};
    }

    const QUrl explicitUrl(trimmedHost);
    if (explicitUrl.isValid() && !explicitUrl.scheme().isEmpty()) {
        if (explicitUrl.scheme() != QStringLiteral("http") && explicitUrl.scheme() != QStringLiteral("https")) {
            return {};
        }
        QUrl url = explicitUrl;
        const QString manifestPath = QString::fromLatin1(amnezia::protocols::selfHostedUpdates::manifestPath);
        if (url.path().isEmpty() || url.path() == QStringLiteral("/")) {
            url.setPath(manifestPath);
        } else if (!url.path().endsWith(manifestPath)) {
            QString path = url.path().trimmed();
            while (path.endsWith(QLatin1Char('/'))) {
                path.chop(1);
            }
            url.setPath(path + manifestPath);
        }
        return url;
    }

    return QUrl(selfHostedUpdateUrl(trimmedHost, QString::fromLatin1(amnezia::protocols::selfHostedUpdates::manifestPath)));
}

QList<QString> UpdateController::platformCandidates() const
{
    const QString arch = QSysInfo::currentCpuArchitecture().toLower();
    QStringList candidates;

#if defined(Q_OS_WINDOWS)
    candidates << QStringLiteral("windows-%1").arg(arch.contains(QStringLiteral("arm")) ? QStringLiteral("arm64") : QStringLiteral("x64"))
               << QStringLiteral("windows-x64")
               << QStringLiteral("windows");
#elif defined(Q_OS_ANDROID)
    if (arch.contains(QStringLiteral("arm64")) || arch.contains(QStringLiteral("aarch64"))) {
        candidates << QStringLiteral("android-arm64-v8a");
    } else if (arch.contains(QStringLiteral("arm"))) {
        candidates << QStringLiteral("android-armeabi-v7a");
    } else if (arch.contains(QStringLiteral("x86_64")) || arch.contains(QStringLiteral("amd64"))) {
        candidates << QStringLiteral("android-x86_64");
    } else if (arch.contains(QStringLiteral("x86"))) {
        candidates << QStringLiteral("android-x86");
    }
    candidates << QStringLiteral("android");
#elif defined(Q_OS_LINUX)
    candidates << QStringLiteral("linux-%1").arg(arch.contains(QStringLiteral("arm")) ? QStringLiteral("arm64") : QStringLiteral("x64"))
               << QStringLiteral("linux-x64")
               << QStringLiteral("linux");
#endif

    candidates.removeDuplicates();
    return candidates;
}

bool UpdateController::processSelfHostedManifest(const QUrl &manifestUrl, const QByteArray &manifestData)
{
    QByteArray payloadData;
    if (!verifySignedManifestEnvelope(manifestData, payloadData)) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payloadData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        logger.error() << "Invalid self-hosted update payload:" << parseError.errorString();
        return false;
    }

    const QJsonObject payload = document.object();
    if (payload.value(QStringLiteral("schema")).toInt() != 1) {
        logger.error() << "Unexpected self-hosted update payload schema";
        return false;
    }

    const QString version = payload.value(QStringLiteral("version")).toString().trimmed();
    if (version.isEmpty() || !isNewVersionAvailable(version)) {
        return false;
    }

    UpdateArtifact artifact;
    if (!selectSelfHostedArtifact(manifestUrl, payload, artifact)) {
        return false;
    }

    m_version = version;
    m_releaseDate = payload.value(QStringLiteral("releaseDate")).toString(
            payload.value(QStringLiteral("release_date")).toString());
    m_changelogText = payload.value(QStringLiteral("changelog")).toString(
            payload.value(QStringLiteral("body")).toString());
    m_selectedArtifact = artifact;
    m_downloadUrl = artifact.url.toString();
    m_baseUrl = manifestUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toString();
    m_useSelfHostedArtifact = true;
    logger.info() << "Self-hosted update available:" << m_version << "for" << artifact.platform << artifact.url.toString();
    return true;
}

bool UpdateController::verifySignedManifestEnvelope(const QByteArray &manifestData, QByteArray &payloadData) const
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(manifestData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        logger.error() << "Invalid self-hosted update manifest:" << parseError.errorString();
        return false;
    }

    const QJsonObject envelope = document.object();
    const QString schema = envelope.value(QStringLiteral("schema")).toString();
    if (schema != QStringLiteral("amnezia-selfhosted-update-v1")) {
        logger.error() << "Unexpected self-hosted update manifest schema:" << schema;
        return false;
    }
    const QString signatureAlgorithm = envelope.value(QStringLiteral("signatureAlgorithm")).toString();
    if (signatureAlgorithm != QStringLiteral("Ed25519")) {
        logger.error() << "Unexpected self-hosted update manifest signature algorithm:" << signatureAlgorithm;
        return false;
    }

    QByteArray payload;
    QByteArray signature;
    const bool decodedPayload = decodeStrictBase64(envelope.value(QStringLiteral("payload")).toString().toUtf8(),
                                                   QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals,
                                                   payload);
    const bool decodedSignature = decodeStrictBase64(envelope.value(QStringLiteral("signature")).toString().toUtf8(),
                                                     QByteArray::Base64Encoding,
                                                     signature);
    if (!decodedPayload || !decodedSignature || payload.isEmpty() || signature.isEmpty()) {
        logger.error() << "Self-hosted update manifest is missing payload or signature";
        return false;
    }
    if (payload.size() > kManifestMaxPayloadBytes) {
        logger.error() << "Self-hosted update manifest payload is too large";
        return false;
    }
    if (signature.size() != 64) {
        logger.error() << "Self-hosted update manifest signature has invalid Ed25519 size";
        return false;
    }
    if (!verifyManifestSignature(payload, signature)) {
        logger.error() << "Self-hosted update manifest signature verification failed";
        return false;
    }

    payloadData = payload;
    return true;
}

bool UpdateController::verifyManifestSignature(const QByteArray &payloadData, const QByteArray &signature) const
{
    QByteArray publicKeyPem;
    if (!decodeStrictBase64(QByteArray(SELFHOSTED_UPDATE_PUBLIC_KEY_PEM_BASE64),
                            QByteArray::Base64Encoding,
                            publicKeyPem) || publicKeyPem.isEmpty()) {
        logger.warning() << "Self-hosted update public key is not configured; ignoring private update manifest";
        return false;
    }

    BIO *bio = BIO_new_mem_buf(publicKeyPem.constData(), publicKeyPem.size());
    if (!bio) {
        return false;
    }

    EVP_PKEY *publicKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!publicKey) {
        return false;
    }

    bool ok = false;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx) {
        if (EVP_PKEY_base_id(publicKey) == EVP_PKEY_ED25519
            && EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, publicKey) == 1
            && EVP_DigestVerify(ctx,
                                reinterpret_cast<const unsigned char *>(signature.constData()),
                                static_cast<size_t>(signature.size()),
                                reinterpret_cast<const unsigned char *>(payloadData.constData()),
                                static_cast<size_t>(payloadData.size())) == 1) {
            ok = true;
        }
        EVP_MD_CTX_free(ctx);
    }
    EVP_PKEY_free(publicKey);
    return ok;
}

bool UpdateController::selectSelfHostedArtifact(const QUrl &manifestUrl, const QJsonObject &payload, UpdateArtifact &artifactOut)
{
    const QJsonObject platforms = payload.value(QStringLiteral("platforms")).toObject();
    if (platforms.isEmpty()) {
        logger.error() << "Self-hosted update payload has no platforms";
        return false;
    }

    for (const QString &platform : platformCandidates()) {
        const QJsonValue artifactValue = platforms.value(platform);
        if (!artifactValue.isObject()) {
            continue;
        }

        const QJsonObject artifactObject = artifactValue.toObject();
        const QString urlOrPath = artifactObject.value(QStringLiteral("url")).toString(
                artifactObject.value(QStringLiteral("path")).toString());
        const QUrl url = resolvedArtifactUrl(manifestUrl, urlOrPath);
        if (!url.isValid() || url.isEmpty()) {
            continue;
        }

        UpdateArtifact artifact;
        artifact.platform = platform;
        artifact.url = url;
        artifact.sha256 = normalizedHexSha256(artifactObject.value(QStringLiteral("sha256")).toString());
        const QJsonValue sizeValue = artifactObject.value(QStringLiteral("size"));
        artifact.size = sizeValue.isDouble() ? sizeValue.toVariant().toLongLong() : -1;
        artifact.openExternally = artifactObject.value(QStringLiteral("openExternal")).toBool(false);
        artifact.autoInstall = artifactObject.value(QStringLiteral("autoInstall")).toBool(
                payload.value(QStringLiteral("autoInstall")).toBool(false));

#if defined(Q_OS_IOS) || defined(MACOS_NE)
        artifact.openExternally = true;
#endif

        if (!artifact.openExternally && !isSha256Hex(artifact.sha256)) {
            logger.error() << "Self-hosted update artifact is missing or has invalid sha256 for" << platform;
            continue;
        }
        if (!artifact.openExternally && !isHttpOrHttpsUrl(artifact.url)) {
            logger.error() << "Self-hosted update artifact URL must use http(s) for" << platform;
            continue;
        }
        if (!artifact.openExternally && artifact.size <= 0) {
            logger.error() << "Self-hosted update artifact is missing or has invalid size for" << platform;
            continue;
        }
        if (artifact.openExternally && !isAllowedExternalUpdateUrl(artifact.url)) {
            logger.error() << "Self-hosted external update URL has unsupported scheme for" << platform;
            continue;
        }

        artifactOut = artifact;
        return true;
    }

    logger.info() << "No matching self-hosted update artifact for platform candidates" << platformCandidates();
    return false;
}

QUrl UpdateController::resolvedArtifactUrl(const QUrl &manifestUrl, const QString &urlOrPath) const
{
    if (urlOrPath.trimmed().isEmpty()) {
        return {};
    }

    const QUrl candidate(urlOrPath);
    if (candidate.isValid() && !candidate.isRelative()) {
        return candidate;
    }
    return manifestUrl.resolved(QUrl(urlOrPath));
}

void UpdateController::setupNetworkErrorHandling(QNetworkReply* reply, const QString& operation)
{
    QObject::connect(reply, &QNetworkReply::errorOccurred, [reply, operation](QNetworkReply::NetworkError error) {
        logger.error() << QString("Network error occurred while fetching %1: %2 %3")
                          .arg(operation, reply->errorString(), QString::number(error));
    });

    QObject::connect(reply, &QNetworkReply::sslErrors, [operation](const QList<QSslError> &errors) {
        QStringList errorStrings;
        for (const QSslError &err : errors) {
            errorStrings << err.errorString();
        }
        logger.error() << QString("SSL errors while fetching %1: %2").arg(operation, errorStrings.join("; "));
    });
}

void UpdateController::handleNetworkError(QNetworkReply* reply, const QString& operation)
{
    logger.error() << "Network error code:" << QString::number(static_cast<int>(reply->error()));
    logger.error() << "HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}

QString UpdateController::composeDownloadUrl() const
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    const QString fileName = QString(kInstallerRemoteFileNamePattern).arg(m_version);
    return m_baseUrl + "/" + fileName;
#else
    return QString();
#endif
}

QString UpdateController::localInstallerPath() const
{
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#if defined(Q_OS_WINDOWS)
    return tempDir + QStringLiteral("/AmneziaVPN_installer.exe");
#elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
    return tempDir + QStringLiteral("/AmneziaVPN.pkg");
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    return tempDir + QStringLiteral("/AmneziaVPN.run");
#elif defined(Q_OS_ANDROID)
    return tempDir + QStringLiteral("/AmneziaVPN.apk");
#else
    return tempDir + QStringLiteral("/AmneziaVPN_update");
#endif
}

void UpdateController::runInstaller()
{
    if (m_useSelfHostedArtifact) {
        if (m_selfHostedInstallInProgress) {
            logger.info() << "Self-hosted update installer handoff is already in progress";
            return;
        }
        m_selfHostedInstallInProgress = true;
    }

    if (m_useSelfHostedArtifact && m_selectedArtifact.openExternally) {
        finishSelfHostedInstallerAttempt(openArtifactExternally()
                        ? InstallerHandoffResult::Started
                        : InstallerHandoffResult::Failed);
        return;
    }

#if defined(Q_OS_ANDROID)
    if (m_useSelfHostedArtifact) {
        startArtifactDownload();
        return;
    }
    openArtifactExternally();
#elif !defined(Q_OS_IOS) && !defined(MACOS_NE)
    if (m_useSelfHostedArtifact) {
        startArtifactDownload();
        return;
    }

    if (m_downloadUrl.isEmpty()) {
        logger.error() << "Download URL is empty";
        return;
    }

    QNetworkRequest request;
    request.setTransferTimeout(30000);
    request.setUrl(m_downloadUrl);

    QNetworkReply *reply = amnApp->networkManager()->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            const QString installerPath = localInstallerPath();
            QFile file(installerPath);
            if (!file.open(QIODevice::WriteOnly)) {
                logger.error() << "Failed to open installer file for writing:" << installerPath << "Error:" << file.errorString();
                reply->deleteLater();
                return;
            }

            if (file.write(reply->readAll()) == -1) {
                logger.error() << "Failed to write installer data to file:" << installerPath << "Error:" << file.errorString();
                file.close();
                reply->deleteLater();
                return;
            }

            file.close();

    #if defined(Q_OS_WINDOWS)
            if (runWindowsInstaller(installerPath) == 0) {
                scheduleDesktopQuitAfterInstallerStart();
            }
    #elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
            if (runMacInstaller(installerPath) == 0) {
                scheduleDesktopQuitAfterInstallerStart();
            }
    #elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
            if (runLinuxInstaller(installerPath) == 0) {
                scheduleDesktopQuitAfterInstallerStart();
            }
    #endif
        } else {
            logger.error() << "Installer download failed, network error:" << static_cast<int>(reply->error())
                           << reply->errorString();
            logger.error() << "HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }
        reply->deleteLater();
    });
#else
    if (m_useSelfHostedArtifact) {
        finishSelfHostedInstallerAttempt(openArtifactExternally()
                        ? InstallerHandoffResult::Started
                        : InstallerHandoffResult::Failed);
    } else {
        openArtifactExternally();
    }
#endif
}

void UpdateController::startArtifactDownload()
{
    if (m_selectedArtifact.url.isEmpty()) {
        logger.error() << "Self-hosted update artifact URL is empty";
        finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
        return;
    }

    const QString installerPath = localInstallerPath();
    if (!QDir().mkpath(QFileInfo(installerPath).absolutePath())) {
        logger.error() << "Failed to create self-hosted installer directory:" << installerPath;
        finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
        return;
    }

    const QString partialPath = installerPath + QStringLiteral(".download");
    QFile::remove(partialPath);

    QFile *file = new QFile(partialPath);
    if (!file->open(QIODevice::WriteOnly)) {
        logger.error() << "Failed to open self-hosted installer for writing:" << partialPath << file->errorString();
        delete file;
        finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
        return;
    }

    auto *hash = new QCryptographicHash(QCryptographicHash::Sha256);
    auto *bytesWritten = new qint64(0);
    auto *writeFailed = new bool(false);
    auto *downloadTooLarge = new bool(false);

    QNetworkRequest request;
    request.setTransferTimeout(kInstallerTransferTimeoutMs);
    request.setUrl(m_selectedArtifact.url);

    QNetworkReply *reply = amnApp->networkManager()->get(request);
    QObject::connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply, downloadTooLarge]() {
        const QVariant contentLength = reply->header(QNetworkRequest::ContentLengthHeader);
        if (m_selectedArtifact.size >= 0 && contentLength.isValid()
            && contentLength.toLongLong() > m_selectedArtifact.size) {
            *downloadTooLarge = true;
            reply->abort();
        }
    });
    QObject::connect(reply, &QIODevice::readyRead, this, [this, reply, file, hash, bytesWritten, writeFailed, downloadTooLarge]() {
        const QByteArray chunk = reply->readAll();
        if (chunk.isEmpty() || *writeFailed || *downloadTooLarge) {
            return;
        }
        if (m_selectedArtifact.size >= 0 && *bytesWritten + chunk.size() > m_selectedArtifact.size) {
            *downloadTooLarge = true;
            reply->abort();
            return;
        }
        if (file->write(chunk) != chunk.size()) {
            *writeFailed = true;
            return;
        }
        hash->addData(chunk);
        *bytesWritten += chunk.size();
    });
    QObject::connect(reply, &QNetworkReply::finished, this,
                     [this, reply, file, hash, bytesWritten, writeFailed, downloadTooLarge, installerPath, partialPath]() {
        const auto cleanup = [file, hash, bytesWritten, writeFailed, downloadTooLarge]() {
            delete file;
            delete hash;
            delete bytesWritten;
            delete writeFailed;
            delete downloadTooLarge;
        };

        if (reply->bytesAvailable() > 0 && !*writeFailed && !*downloadTooLarge) {
            const QByteArray chunk = reply->readAll();
            if (!chunk.isEmpty()) {
                if (m_selectedArtifact.size >= 0 && *bytesWritten + chunk.size() > m_selectedArtifact.size) {
                    *downloadTooLarge = true;
                } else if (file->write(chunk) != chunk.size()) {
                    *writeFailed = true;
                } else {
                    hash->addData(chunk);
                    *bytesWritten += chunk.size();
                }
            }
        }
        file->close();

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
            logger.error() << "Self-hosted installer download failed:" << static_cast<int>(reply->error())
                           << reply->errorString() << "HTTP status:" << statusCode;
            reply->deleteLater();
            QFile::remove(partialPath);
            cleanup();
            finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
            return;
        }

        reply->deleteLater();

        if (*downloadTooLarge) {
            logger.error() << "Self-hosted installer download exceeded manifest size for" << m_selectedArtifact.url.toString();
            QFile::remove(partialPath);
            cleanup();
            finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
            return;
        }

        if (*writeFailed) {
            logger.error() << "Failed to write full self-hosted installer:" << partialPath << file->errorString();
            QFile::remove(partialPath);
            cleanup();
            finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
            return;
        }

        const QString actualSha256 = QString::fromLatin1(hash->result().toHex());
        if (actualSha256 != normalizedHexSha256(m_selectedArtifact.sha256)) {
            logger.error() << "Self-hosted installer sha256 verification failed for" << m_selectedArtifact.url.toString();
            QFile::remove(partialPath);
            cleanup();
            finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
            return;
        }
        if (m_selectedArtifact.size >= 0 && *bytesWritten != m_selectedArtifact.size) {
            logger.error() << "Self-hosted installer size differs from manifest:"
                             << *bytesWritten << "expected" << m_selectedArtifact.size;
            QFile::remove(partialPath);
            cleanup();
            finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
            return;
        }

        QFile::remove(installerPath);
        if (!QFile::rename(partialPath, installerPath)) {
            logger.error() << "Failed to move verified self-hosted installer into place:" << installerPath;
            QFile::remove(partialPath);
            cleanup();
            finishSelfHostedInstallerAttempt(InstallerHandoffResult::Failed);
            return;
        }

        cleanup();
        finishSelfHostedInstallerAttempt(launchDownloadedArtifact(installerPath));
    });
}

UpdateController::InstallerHandoffResult UpdateController::launchDownloadedArtifact(const QString &localPath)
{
#if defined(Q_OS_WINDOWS)
    const bool launched = runWindowsInstaller(localPath) == 0;
    if (launched) {
        scheduleDesktopQuitAfterInstallerStart();
    }
    return launched ? InstallerHandoffResult::Started : InstallerHandoffResult::Failed;
#elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
    const bool launched = runMacInstaller(localPath) == 0;
    if (launched) {
        scheduleDesktopQuitAfterInstallerStart();
    }
    return launched ? InstallerHandoffResult::Started : InstallerHandoffResult::Failed;
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    const bool launched = runLinuxInstaller(localPath) == 0;
    if (launched) {
        scheduleDesktopQuitAfterInstallerStart();
    }
    return launched ? InstallerHandoffResult::Started : InstallerHandoffResult::Failed;
#elif defined(Q_OS_ANDROID)
    const int result = AndroidController::instance()->installApk(localPath);
    if (result == kAndroidApkInstallStarted) {
        return InstallerHandoffResult::Started;
    }
    if (result == kAndroidApkInstallPermissionSettingsOpened) {
        return InstallerHandoffResult::PendingPermission;
    }
    return InstallerHandoffResult::Failed;
#else
    return QDesktopServices::openUrl(QUrl::fromLocalFile(localPath))
            ? InstallerHandoffResult::Started
            : InstallerHandoffResult::Failed;
#endif
}

bool UpdateController::openArtifactExternally()
{
    const QUrl url = m_selectedArtifact.url.isEmpty() ? QUrl(m_downloadUrl) : m_selectedArtifact.url;
    if (!url.isValid() || url.isEmpty()) {
        logger.error() << "Update URL is empty or invalid";
        return false;
    }
    if (!isAllowedExternalUpdateUrl(url)) {
        logger.error() << "Update URL has unsupported external scheme:" << url.toString();
        return false;
    }
    if (!QDesktopServices::openUrl(url)) {
        logger.error() << "Failed to open update URL externally:" << url.toString();
        return false;
    }
    return true;
}

bool UpdateController::shouldAutoInstallSelfHostedArtifact() const
{
    if (!m_useSelfHostedArtifact || !m_selectedArtifact.autoInstall || !m_appSettingsRepository) {
        return false;
    }
    const QString attemptMarker = selfHostedAutoInstallAttemptMarker();
    return !attemptMarker.isEmpty() && m_appSettingsRepository->selfHostedUpdateLastAutoInstallAttempt() != attemptMarker;
}

QString UpdateController::selfHostedAutoInstallAttemptId() const
{
    if (m_version.trimmed().isEmpty() || m_selectedArtifact.platform.trimmed().isEmpty()) {
        return {};
    }
    const QString artifactIdentity = m_selectedArtifact.sha256.isEmpty()
            ? m_selectedArtifact.url.toString()
            : m_selectedArtifact.sha256;
    return QStringLiteral("%1|%2|%3").arg(m_version, m_selectedArtifact.platform, artifactIdentity);
}

QString UpdateController::selfHostedAutoInstallAttemptMarker() const
{
    const QString attemptId = selfHostedAutoInstallAttemptId();
    if (attemptId.isEmpty()) {
        return {};
    }
    return QStringLiteral("%1|%2").arg(attemptId, QDate::currentDate().toString(Qt::ISODate));
}

void UpdateController::scheduleSelfHostedAutoInstall()
{
    if (!shouldAutoInstallSelfHostedArtifact()) {
        return;
    }

    m_pendingAutoInstallAttemptId = selfHostedAutoInstallAttemptMarker();
    QTimer::singleShot(0, this, [this]() {
        runInstaller();
    });
}

void UpdateController::commitPendingAutoInstallAttempt()
{
    if (!m_appSettingsRepository || m_pendingAutoInstallAttemptId.isEmpty()) {
        return;
    }
    m_appSettingsRepository->setSelfHostedUpdateLastAutoInstallAttempt(m_pendingAutoInstallAttemptId);
    m_pendingAutoInstallAttemptId.clear();
}

void UpdateController::clearPendingAutoInstallAttempt()
{
    m_pendingAutoInstallAttemptId.clear();
}

void UpdateController::finishSelfHostedInstallerAttempt(InstallerHandoffResult result)
{
    if (result == InstallerHandoffResult::Started) {
        commitPendingAutoInstallAttempt();
        m_androidApkInstallPermissionPending = false;
        m_selfHostedInstallInProgress = false;
        return;
    }

    if (result == InstallerHandoffResult::PendingPermission) {
        m_androidApkInstallPermissionPending = true;
        const QString attemptId = m_pendingAutoInstallAttemptId;
        QTimer::singleShot(kAndroidApkInstallPermissionWaitMs, this, [this, attemptId]() {
            if (!m_androidApkInstallPermissionPending || m_pendingAutoInstallAttemptId != attemptId) {
                return;
            }
            logger.info() << "Android APK install permission handoff timed out";
            m_androidApkInstallPermissionPending = false;
            m_selfHostedInstallInProgress = false;
            clearPendingAutoInstallAttempt();
        });
        return;
    }

    m_androidApkInstallPermissionPending = false;
    m_selfHostedInstallInProgress = false;
    clearPendingAutoInstallAttempt();
}

void UpdateController::onAndroidApkInstallerStarted(const QString &fileName)
{
#if defined(Q_OS_ANDROID)
    if (!m_androidApkInstallPermissionPending && m_pendingAutoInstallAttemptId.isEmpty()) {
        return;
    }

    if (QFileInfo(fileName).absoluteFilePath() != QFileInfo(localInstallerPath()).absoluteFilePath()) {
        logger.info() << "Ignoring Android APK installer callback for unrelated file:" << fileName;
        return;
    }

    m_androidApkInstallPermissionPending = false;
    m_selfHostedInstallInProgress = false;
    if (!m_pendingAutoInstallAttemptId.isEmpty()) {
        commitPendingAutoInstallAttempt();
    }
#else
    Q_UNUSED(fileName);
#endif
}

void UpdateController::scheduleDesktopQuitAfterInstallerStart()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    QTimer::singleShot(kDesktopQuitAfterInstallerStartMs, this, []() {
        if (amnApp) {
            logger.info() << "Quitting application after update installer handoff";
            amnApp->forceQuit();
        }
    });
#endif
}

#if defined(Q_OS_WINDOWS)
int UpdateController::runWindowsInstaller(const QString &installerPath)
{
    qint64 pid;
    bool success = QProcess::startDetached(installerPath, QStringList(), QString(), &pid);

    if (success) {
        logger.info() << "Installation process started with PID:" << pid;
    } else {
        logger.error() << "Failed to start installation process";
        return -1;
    }

    return 0;
}
#endif

#if defined(Q_OS_MACOS) && !defined(MACOS_NE)
int UpdateController::runMacInstaller(const QString &installerPath)
{
    // Create temporary directory for extraction
    QTemporaryDir extractDir;
    extractDir.setAutoRemove(false);
    if (!extractDir.isValid()) {
        logger.error() << "Failed to create temporary directory";
        return -1;
    }
    logger.info() << "Temporary directory created:" << extractDir.path();

    // Create script file in the temporary directory
    QString scriptPath = extractDir.path() + "/mac_installer.sh";
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly)) {
        logger.error() << "Failed to create script file";
        return -1;
    }

    // Get script content from registry
    QString scriptContent = amnezia::scriptData(amnezia::ClientScriptType::mac_installer);
    if (scriptContent.isEmpty()) {
        logger.error() << "macOS installer script content is empty";
        scriptFile.close();
        return -1;
    }

    scriptFile.write(scriptContent.toUtf8());
    scriptFile.close();
    logger.info() << "Script file created:" << scriptPath;

    // Make script executable
    QFile::setPermissions(scriptPath, QFile::permissions(scriptPath) | QFile::ExeUser);

    // Start detached process
    qint64 pid;
    bool success =
            QProcess::startDetached("/bin/bash", QStringList() << scriptPath << extractDir.path() << installerPath, extractDir.path(), &pid);

    if (success) {
        logger.info() << "Installation process started with PID:" << pid;
    } else {
        logger.error() << "Failed to start installation process";
        return -1;
    }

    return 0;
}
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
int UpdateController::runLinuxInstaller(const QString &installerPath)
{
    QFile::setPermissions(installerPath, QFile::permissions(installerPath) | QFile::ExeUser);

    qint64 pid;
    bool success = QProcess::startDetached(installerPath, QStringList(), QString(), &pid);

    if (success) {
        logger.info() << "Installation process started with PID:" << pid;
    } else {
        logger.error() << "Failed to start installation process";
        return -1;
    }

    return 0;
}
#endif
