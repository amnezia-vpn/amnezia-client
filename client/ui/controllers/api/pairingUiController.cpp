#include "pairingUiController.h"

#include <QDataStream>
#include <QDebug>
#include <QIODevice>
#include <QRegularExpression>
#include <QTimer>
#include <QUuid>

#if defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

#include "core/controllers/gatewayController.h"
#include "core/models/serverConfig.h"
#include "core/models/api/apiV2ServerConfig.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/qrCodeUtils.h"

using namespace amnezia;

namespace
{
constexpr auto kGenerateQrPath = "%1api/v1/generate_qr";
constexpr auto kScanQrPath = "%1api/v1/scan_qr";
constexpr int kPairingRetryMaxAttempts = 3;

bool isPairingRetriableError(ErrorCode code)
{
    switch (code) {
    case ErrorCode::ApiPairingRateLimitedError:
    case ErrorCode::ApiPairingServiceUnavailableError:
    case ErrorCode::ApiConfigDownloadError:
        return true;
    default:
        return false;
    }
}

int pairingRetryDelayMs(int zeroBasedAttempt)
{
    constexpr int baseMs = 500;
    return baseMs * (1 << zeroBasedAttempt);
}

/** Legacy TV QR: generateQrCodeImageSeries base64url-wrapped QDataStream chunk (see qrCodeUtils.cpp). */
bool tryDecodeLegacyChunkedPairingQrPayload(const QString &t, QString *outUuid)
{
    static const QRegularExpression binUrlSafe(QStringLiteral("^[A-Za-z0-9_-]+$"));
    if (!binUrlSafe.match(t).hasMatch() || t.size() < 16) {
        return false;
    }
    QByteArray padded = t.toUtf8();
    switch (padded.size() % 4) {
    case 2:
        padded += "==";
        break;
    case 3:
        padded += "=";
        break;
    default:
        break;
    }
    const QByteArray raw = QByteArray::fromBase64(padded, QByteArray::Base64UrlEncoding);
    if (raw.isEmpty()) {
        return false;
    }
    QDataStream ds(raw);
    ds.setByteOrder(QDataStream::BigEndian);
    qint16 magic = 0;
    quint8 nChunks = 0;
    quint8 chunkIndex = 0;
    QByteArray pl;
    ds >> magic >> nChunks >> chunkIndex >> pl;
    if (ds.status() != QDataStream::Ok) {
        return false;
    }
    if (magic != qrCodeUtils::qrMagicCode || nChunks < 1 || nChunks > 200 || chunkIndex >= nChunks) {
        return false;
    }
    const QString candidate = QString::fromUtf8(pl).trimmed();
    const QUuid u = QUuid::fromString(candidate);
    if (u.isNull()) {
        return false;
    }
    *outUuid = u.toString(QUuid::WithoutBraces);
    return true;
}
} // namespace

#if defined(Q_OS_ANDROID)
namespace {
PairingUiController *g_pairingUiForAndroidQr = nullptr;
}
#endif

PairingUiController::PairingUiController(PairingController *pairingController, ServersController *serversController,
                                         SubscriptionController *subscriptionController,
                                         SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent),
      m_pairingController(pairingController),
      m_serversController(serversController),
      m_subscriptionController(subscriptionController),
      m_appSettingsRepository(appSettingsRepository)
{
#if defined(Q_OS_ANDROID)
    g_pairingUiForAndroidQr = this;
#endif
}

PairingUiController::~PairingUiController()
{
#if defined(Q_OS_ANDROID)
    if (g_pairingUiForAndroidQr == this) {
        g_pairingUiForAndroidQr = nullptr;
    }
#endif
}

void PairingUiController::setTvPairingUiPhase(int phase)
{
    if (m_tvPairingUiPhase == phase) {
        return;
    }
    m_tvPairingUiPhase = phase;
    emit tvPairingUiPhaseChanged();
}

void PairingUiController::openPairingQrScanner()
{
#if defined(Q_OS_ANDROID)
    qInfo() << "[PairingUi] openPairingQrScanner (Android native activity)";
    AndroidController::instance()->startQrReaderActivity();
#endif
}

bool PairingUiController::applyScannedTextAsPairingUuid(const QString &raw)
{
    const QString t = raw.trimmed();
    qInfo() << "[PairingUi] scan raw len=" << t.size();
    if (t.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
        qInfo() << "[PairingUi] scan rejected: looks like vpn:// bundle, not session UUID";
        return false;
    }
    static const QRegularExpression reV4(QStringLiteral(
            "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}"));
    const QRegularExpressionMatch m = reV4.match(t);
    if (m.hasMatch()) {
        const QString uuid = m.captured(0);
        qInfo() << "[PairingUi] scan accepted uuid=" << uuid.left(13) << "...";
        emit pairingUuidFromScan(uuid);
        return true;
    }
    QString fromLegacy;
    if (tryDecodeLegacyChunkedPairingQrPayload(t, &fromLegacy)) {
        qInfo() << "[PairingUi] scan accepted legacy chunked QR uuid=" << fromLegacy.left(13) << "...";
        emit pairingUuidFromScan(fromLegacy);
        return true;
    }
    const QUuid parsed = QUuid::fromString(t);
    if (!parsed.isNull()) {
        const QString canon = parsed.toString(QUuid::WithoutBraces);
        qInfo() << "[PairingUi] scan accepted QUuid::fromString uuid=" << canon.left(13) << "...";
        emit pairingUuidFromScan(canon);
        return true;
    }
    qInfo() << "[PairingUi] scan rejected: no session UUID recognized in payload";
    return false;
}

#if defined(Q_OS_ANDROID)
bool PairingUiController::tryConsumeAndroidQrScan(const QString &code)
{
    if (!g_pairingUiForAndroidQr) {
        return false;
    }
    return g_pairingUiForAndroidQr->applyScannedTextAsPairingUuid(code);
}
#endif

QVariantList PairingUiController::tvQrCodes() const
{
    QVariantList list;
    list.reserve(m_tvQrCodes.size());
    for (const QString &s : m_tvQrCodes) {
        list.append(s);
    }
    return list;
}

int PairingUiController::tvQrCodesCount() const
{
    return m_tvQrCodes.size();
}

QString PairingUiController::tvSessionUuid() const
{
    return m_tvSessionUuid;
}

bool PairingUiController::tvPairingBusy() const
{
    return m_tvPairingBusy;
}

QString PairingUiController::tvStatusMessage() const
{
    return m_tvStatusMessage;
}

int PairingUiController::tvPairingWaitWindowSeconds() const
{
    if (!m_pairingController) {
        return 30;
    }
    const int msec = m_pairingController->pairingLongPollTimeoutMsecs();
    return qMax(1, (msec + 999) / 1000);
}

bool PairingUiController::phonePairingBusy() const
{
    return m_phonePairingBusy;
}

QString PairingUiController::phoneStatusMessage() const
{
    return m_phoneStatusMessage;
}

void PairingUiController::setTvBusy(bool busy)
{
    if (m_tvPairingBusy == busy) {
        return;
    }
    m_tvPairingBusy = busy;
    emit tvPairingBusyChanged();
}

void PairingUiController::setPhoneBusy(bool busy)
{
    if (m_phonePairingBusy == busy) {
        return;
    }
    m_phonePairingBusy = busy;
    emit phonePairingBusyChanged();
}

void PairingUiController::resetTvQrDisplay()
{
    m_tvQrCodes.clear();
    m_tvSessionUuid.clear();
    emit tvQrCodesChanged();
    emit tvSessionUuidChanged();
}

QString PairingUiController::tvFailureMessage(ErrorCode code) const
{
    switch (code) {
    case ErrorCode::ApiConfigTimeoutError:
        return tr("QR session expired. Tap Start to show a new QR code.");
    case ErrorCode::ApiConfigAlreadyAdded:
        return tr("This configuration is already on the device.");
    case ErrorCode::ApiNotFoundError:
        return tr("This gateway does not expose QR pairing (HTTP 404). Check the gateway URL or use the local mock (tools/local_gateway).");
    default:
        return tr("Pairing failed");
    }
}

void PairingUiController::startTvQrSession()
{
    if (!m_pairingController || !m_appSettingsRepository) {
        return;
    }
    if (m_tvPairingBusy) {
        return;
    }

    if (m_tvWatcher) {
        m_tvWatcher->disconnect();
        m_tvWatcher->deleteLater();
        m_tvWatcher.clear();
    }

    ++m_tvSessionGeneration;
    const quint64 generation = m_tvSessionGeneration;

    m_tvSessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QByteArray qrPayload = m_tvSessionUuid.toUtf8();
    m_tvQrCodes = qrCodeUtils::generateQrCodeImageSeriesPlainText(qrPayload);
    emit tvQrCodesChanged();
    emit tvSessionUuidChanged();

    m_tvStatusMessage = tr("Waiting for premium device to confirm…");
    emit tvStatusMessageChanged();

    setTvBusy(true);
    setTvPairingUiPhase(1);

    dispatchTvGenerateQrAttempt(generation, 0);
}

void PairingUiController::dispatchTvGenerateQrAttempt(quint64 generation, int retryAttempt)
{
    if (!m_pairingController || !m_appSettingsRepository) {
        return;
    }
    if (generation != m_tvSessionGeneration) {
        return;
    }

    const bool isTestPurchase = false;
    auto gatewayController = QSharedPointer<GatewayController>::create(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                                                       m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                                                       m_pairingController->pairingLongPollTimeoutMsecs(),
                                                                       m_appSettingsRepository->isStrictKillSwitchEnabled());

    const QJsonObject payload = m_pairingController->buildGenerateQrPayload(m_tvSessionUuid);
    QNetworkReply *replyRaw = nullptr;
    const QFuture<QPair<ErrorCode, QByteArray>> future =
            gatewayController->postAsync(QString::fromLatin1(kGenerateQrPath), payload, &replyRaw);
    m_tvNetworkReply = replyRaw;

    auto *watcher = new QFutureWatcher<QPair<ErrorCode, QByteArray>>(this);
    m_tvWatcher = watcher;
    QObject::connect(watcher, &QFutureWatcher<QPair<ErrorCode, QByteArray>>::finished, this,
                     [this, gatewayController, watcher, generation, retryAttempt]() {
                         Q_UNUSED(gatewayController);
                         const auto result = watcher->result();
                         watcher->deleteLater();
                         if (m_tvWatcher == watcher) {
                             m_tvWatcher.clear();
                         }

                         if (generation != m_tvSessionGeneration) {
                             return;
                         }

                         m_tvNetworkReply.clear();

                         PairingController::QrPairingConfigPayload out;
                         ErrorCode logicalErr = result.first;
                         if (logicalErr == ErrorCode::NoError) {
                             logicalErr = PairingController::parseGenerateQrResponseBody(result.second, out);
                         }

                         if (logicalErr == ErrorCode::NoError) {
                             ServerConfig importedConfig;
                             const ErrorCode impErr = m_subscriptionController->importServerFromQrPairingResponse(
                                     out.config, out.serviceInfo, out.supportedProtocols, importedConfig);
                             Q_UNUSED(importedConfig);
                             setTvBusy(false);
                             if (impErr != ErrorCode::NoError) {
                                 setTvPairingUiPhase(2);
                                 m_tvStatusMessage = tvFailureMessage(impErr);
                                 emit tvStatusMessageChanged();
                                 emit errorOccurred(impErr);
                                 resetTvQrDisplay();
                                 return;
                             }
                             resetTvQrDisplay();
                             m_tvStatusMessage = tr("Configuration received");
                             emit tvStatusMessageChanged();
                             emit tvPairingConfigReceived();
                             setTvPairingUiPhase(0);
                             return;
                         }

                         if (isPairingRetriableError(logicalErr) && retryAttempt + 1 < kPairingRetryMaxAttempts) {
                             const int delayMs = pairingRetryDelayMs(retryAttempt);
                             QTimer::singleShot(delayMs, this, [this, generation, retryAttempt]() {
                                 if (generation != m_tvSessionGeneration) {
                                     return;
                                 }
                                 dispatchTvGenerateQrAttempt(generation, retryAttempt + 1);
                             });
                             return;
                         }

                         setTvBusy(false);
                         setTvPairingUiPhase(logicalErr == ErrorCode::ApiConfigTimeoutError ? 3 : 2);
                         m_tvStatusMessage = tvFailureMessage(logicalErr);
                         emit tvStatusMessageChanged();
                         emit errorOccurred(logicalErr);
                     });
    watcher->setFuture(future);
}

void PairingUiController::cancelTvQrSession()
{
    ++m_tvSessionGeneration;
    if (m_tvNetworkReply) {
        m_tvNetworkReply->abort();
    }
    m_tvNetworkReply.clear();
    if (m_tvWatcher) {
        m_tvWatcher->disconnect();
        m_tvWatcher->deleteLater();
        m_tvWatcher.clear();
    }
    setTvBusy(false);
    m_tvStatusMessage.clear();
    emit tvStatusMessageChanged();
    resetTvQrDisplay();
    setTvPairingUiPhase(0);
}

void PairingUiController::cancelAllPairingActivity()
{
    ++m_phoneSessionGeneration;
    if (m_phoneNetworkReply) {
        m_phoneNetworkReply->abort();
    }
    m_phoneNetworkReply.clear();
    if (m_phoneWatcher) {
        m_phoneWatcher->disconnect();
        m_phoneWatcher->deleteLater();
        m_phoneWatcher.clear();
    }
    setPhoneBusy(false);
    m_phoneStatusMessage.clear();
    emit phoneStatusMessageChanged();

    cancelTvQrSession();
}

void PairingUiController::submitPhonePairing(const QString &qrUuid, int serverIndex)
{
    if (!m_pairingController || !m_serversController || !m_subscriptionController || !m_appSettingsRepository) {
        return;
    }
    if (m_phonePairingBusy) {
        return;
    }

    const QString trimmedUuid = qrUuid.trimmed();
    qInfo() << "[PairingUi] submitPhonePairing serverIndex=" << serverIndex << "uuidLen=" << trimmedUuid.size();
    if (trimmedUuid.isEmpty()) {
        qWarning() << "[PairingUi] submitPhonePairing aborted: empty UUID (paste or scan first)";
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return;
    }

    if (serverIndex < 0 || serverIndex >= m_serversController->getServersCount()) {
        qWarning() << "[PairingUi] submitPhonePairing invalid serverIndex";
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    const ServerConfig serverConfig = m_serversController->getServerConfig(serverIndex);
    if (!serverConfig.isApiV2()) {
        qWarning() << "[PairingUi] submitPhonePairing server is not API v2";
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    const ApiV2ServerConfig *apiV2 = serverConfig.as<ApiV2ServerConfig>();
    if (!apiV2) {
        qWarning() << "[PairingUi] submitPhonePairing cast to ApiV2ServerConfig failed";
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    QString vpnKey;
    const ErrorCode keyErr = m_subscriptionController->prepareVpnKeyExport(serverIndex, vpnKey);
    if (keyErr != ErrorCode::NoError) {
        qWarning() << "[PairingUi] prepareVpnKeyExport failed" << static_cast<int>(keyErr);
        emit errorOccurred(keyErr);
        return;
    }

    const QJsonObject serviceInfo = apiV2->apiConfig.serviceInfo.toJson();
    const QJsonArray supportedProtocols = apiV2->apiConfig.supportedProtocols;
    const QString apiKey = apiV2->authData.apiKey;
    if (apiKey.isEmpty()) {
        qWarning() << "[PairingUi] submitPhonePairing aborted: empty API key on server card";
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return;
    }

    const ErrorCode fieldErr = PairingController::validatePairingScanFields(trimmedUuid, vpnKey, apiKey);
    if (fieldErr != ErrorCode::NoError) {
        emit errorOccurred(fieldErr);
        return;
    }

    ++m_phoneSessionGeneration;
    const quint64 phoneGeneration = m_phoneSessionGeneration;

    m_phoneStatusMessage = tr("Sending…");
    emit phoneStatusMessageChanged();
    setPhoneBusy(true);

    dispatchPhoneScanQrAttempt(trimmedUuid, apiV2->apiConfig.isTestPurchase, vpnKey, serviceInfo, supportedProtocols, apiKey,
                               phoneGeneration, 0);
}

void PairingUiController::dispatchPhoneScanQrAttempt(const QString &qrUuid, const bool isTestPurchase, const QString &vpnKey,
                                                     const QJsonObject &serviceInfo, const QJsonArray &supportedProtocols,
                                                     const QString &apiKey, quint64 generation, int retryAttempt)
{
    if (!m_pairingController || !m_appSettingsRepository) {
        return;
    }
    if (generation != m_phoneSessionGeneration) {
        return;
    }

    auto gatewayController = QSharedPointer<GatewayController>::create(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                                                       m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                                                       apiDefs::requestTimeoutMsecs,
                                                                       m_appSettingsRepository->isStrictKillSwitchEnabled());

    const QJsonObject payload = m_pairingController->buildScanQrPayload(qrUuid, vpnKey, serviceInfo, supportedProtocols, apiKey);
    QNetworkReply *replyRaw = nullptr;
    const QFuture<QPair<ErrorCode, QByteArray>> future =
            gatewayController->postAsync(QString::fromLatin1(kScanQrPath), payload, &replyRaw);
    m_phoneNetworkReply = replyRaw;

    auto *watcher = new QFutureWatcher<QPair<ErrorCode, QByteArray>>(this);
    m_phoneWatcher = watcher;
    QObject::connect(watcher, &QFutureWatcher<QPair<ErrorCode, QByteArray>>::finished, this,
                     [this, gatewayController, watcher, generation, retryAttempt, qrUuid, isTestPurchase, vpnKey, serviceInfo,
                      supportedProtocols, apiKey]() {
                         Q_UNUSED(gatewayController);
                         const auto result = watcher->result();
                         watcher->deleteLater();
                         if (m_phoneWatcher == watcher) {
                             m_phoneWatcher.clear();
                         }

                         if (generation != m_phoneSessionGeneration) {
                             return;
                         }

                         m_phoneNetworkReply.clear();

                         ErrorCode logicalErr = result.first;
                         if (logicalErr == ErrorCode::NoError) {
                             logicalErr = PairingController::parseScanQrResponseBody(result.second);
                         }

                         if (logicalErr == ErrorCode::NoError) {
                             setPhoneBusy(false);
                             m_phoneStatusMessage = tr("Sent successfully");
                             emit phoneStatusMessageChanged();
                             emit phonePairingSucceeded();
                             return;
                         }

                         if (isPairingRetriableError(logicalErr) && retryAttempt + 1 < kPairingRetryMaxAttempts) {
                             const int delayMs = pairingRetryDelayMs(retryAttempt);
                             QTimer::singleShot(delayMs, this, [this, qrUuid, isTestPurchase, vpnKey, serviceInfo, supportedProtocols,
                                                              apiKey, generation, retryAttempt]() {
                                 if (generation != m_phoneSessionGeneration) {
                                     return;
                                 }
                                 dispatchPhoneScanQrAttempt(qrUuid, isTestPurchase, vpnKey, serviceInfo, supportedProtocols, apiKey,
                                                            generation, retryAttempt + 1);
                             });
                             return;
                         }

                         setPhoneBusy(false);
                         m_phoneStatusMessage = tr("Send failed");
                         emit phoneStatusMessageChanged();
                         emit errorOccurred(logicalErr);
                     });
    watcher->setFuture(future);
}
