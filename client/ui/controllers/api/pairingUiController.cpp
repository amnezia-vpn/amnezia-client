#include "pairingUiController.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QIODevice>
#include <QMetaObject>
#include <QPointer>
#include <QRegularExpression>
#include <QTimer>
#include <QUuid>
#include <string>

#include "platforms/ios/iosPairingCameraAccess.h"
#if defined(Q_OS_IOS)
    #include "platforms/ios/iosPairingQrOverlayWindow.h"
#endif

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
constexpr auto kGatewayProbePath = "%1v1/news";
constexpr int kPairingRetryMaxAttempts = 3;
constexpr int kGatewayProbeTimeoutMsecs = 3000;

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

/**
 * Extract a pairing session UUID from raw QR text without touching QObject / signals.
 * Safe from CameraX / JNI threads while AmneziaActivity is stopped (Qt event loop may not run).
 */
QString extractPairingSessionUuidFromScanText(const QString &raw)
{
    const QString t = raw.trimmed();
    if (t.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
        return {};
    }
    static const QRegularExpression reV4(QStringLiteral(
            "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}"));
    const QRegularExpressionMatch m = reV4.match(t);
    if (m.hasMatch()) {
        return m.captured(0);
    }
    QString fromLegacy;
    if (tryDecodeLegacyChunkedPairingQrPayload(t, &fromLegacy)) {
        return fromLegacy;
    }
    const QUuid parsed = QUuid::fromString(t);
    if (!parsed.isNull()) {
        return parsed.toString(QUuid::WithoutBraces);
    }
    return {};
}
} // namespace

#if defined(Q_OS_ANDROID)
namespace {
PairingUiController *g_pairingUiForAndroidQr = nullptr;
}
#endif

bool PairingUiController::iosNativePairingQrOverlayBuild() const
{
#if defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

bool PairingUiController::androidNativePairingQrOverlayBuild() const
{
#if defined(Q_OS_ANDROID)
    return true;
#else
    return false;
#endif
}

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
    connect(AndroidController::instance(), &AndroidController::cameraPermissionResult, this,
            [this](bool granted) { emit pairingCameraAccessFinished(granted); });
#endif
}

PairingUiController::~PairingUiController()
{
#if defined(Q_OS_ANDROID)
    if (g_pairingUiForAndroidQr == this) {
        g_pairingUiForAndroidQr = nullptr;
    }
#endif
#if defined(Q_OS_IOS)
    amneziaIosPairingQrOverlayDismiss();
#endif
}

void PairingUiController::setPendingPhonePairingUuid(const QString &uuid)
{
    const QString trimmed = uuid.trimmed();
    if (m_pendingPhonePairingUuid == trimmed) {
        return;
    }
    m_pendingPhonePairingUuid = trimmed;
    emit pendingPhonePairingUuidChanged();
}

void PairingUiController::clearPendingPhonePairingUuid()
{
    if (m_pendingPhonePairingUuid.isEmpty()) {
        return;
    }
    m_pendingPhonePairingUuid.clear();
    emit pendingPhonePairingUuidChanged();
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
    AndroidController::instance()->startPairingQrReaderActivity();
#endif
}

bool PairingUiController::isPairingCameraAccessGranted() const
{
#if defined(Q_OS_ANDROID)
    return AndroidController::instance()->isCameraPermissionGranted();
#elif defined(Q_OS_IOS)
    const bool ok = amneziaIosPairingCameraAccessGranted();
    qInfo() << "[PairingUi] iOS isPairingCameraAccessGranted =" << ok;
    return ok;
#else
    return true;
#endif
}

void PairingUiController::requestPairingCameraAccess()
{
#if defined(Q_OS_ANDROID)
    AndroidController::instance()->requestCameraPermissionForQrPairing();
#elif defined(Q_OS_IOS)
    amneziaIosRequestPairingCameraAccess([this](bool granted) {
        qInfo() << "[PairingUi] iOS requestPairingCameraAccess callback granted =" << granted;
        QMetaObject::invokeMethod(
                this, [this, granted]() { emit pairingCameraAccessFinished(granted); }, Qt::QueuedConnection);
    });
#else
    emit pairingCameraAccessFinished(true);
#endif
}

void PairingUiController::openPairingCameraAppSettings()
{
#if defined(Q_OS_ANDROID)
    AndroidController::instance()->openApplicationDetailsSettings();
#elif defined(Q_OS_IOS)
    amneziaIosOpenApplicationSettings();
#endif
}

void PairingUiController::setEmbeddedPairingQrCameraActive(bool active)
{
    if (m_embeddedPairingQrCameraActive == active) {
        return;
    }
    m_embeddedPairingQrCameraActive = active;
#if defined(Q_OS_IOS)
    qInfo() << "[PairingUi] iOS embeddedPairingQrCameraActive ->" << active;
    amneziaIosApplyEmbeddedCameraUnderlayToQtView(active);
#endif
#if defined(Q_OS_ANDROID)
    if (active) {
        AndroidController::instance()->startPairingQrEmbeddedCamera();
    } else {
        AndroidController::instance()->stopPairingQrEmbeddedCamera();
    }
#endif
    emit embeddedPairingQrCameraActiveChanged();
}

void PairingUiController::syncIosEmbeddedPairingQrNativeBottomExtra(int extraPt)
{
#if defined(Q_OS_IOS)
    amneziaIosSetPairingEmbeddedCameraNativeBottomExtraPt(extraPt);
#else
    Q_UNUSED(extraPt);
#endif
}

void PairingUiController::refreshIosEmbeddedPairingQrChrome()
{
#if defined(Q_OS_IOS)
    if (!m_embeddedPairingQrCameraActive) {
        return;
    }
    qInfo() << "[PairingUi] refreshIosEmbeddedPairingQrChrome (reapply UIView underlay)";
    amneziaIosApplyEmbeddedCameraUnderlayToQtView(true);
#else
    // no-op on non-iOS
#endif
}

void PairingUiController::setPairingQrTorchEnabled(bool enabled)
{
#if defined(Q_OS_ANDROID)
    AndroidController::instance()->setPairingQrEmbeddedTorch(enabled);
#elif defined(Q_OS_IOS)
    amneziaIosPairingQrOverlaySetTorchEnabled(enabled);
#else
    Q_UNUSED(enabled);
#endif
}

void PairingUiController::presentIosPairingQrNativeOverlayScanner(const QString &title, const QString &subtitle)
{
#if defined(Q_OS_IOS)
    qInfo() << "[PairingUi] presentIosPairingQrNativeOverlayScanner: scheduling native UIWindow overlay";
    const std::string titleUtf8 = title.isEmpty() ? std::string() : title.toStdString();
    const std::string subtitleUtf8 = subtitle.isEmpty() ? std::string() : subtitle.toStdString();
    amneziaIosPairingQrOverlayPresent(
            [this](const char *utf8) {
                const QString code = QString::fromUtf8(utf8);
                QMetaObject::invokeMethod(
                        this,
                        [this, code]() {
                            if (!applyScannedTextAsPairingUuid(code)) {
                                emit pairingSendQrScanRejectedInvalidPayload();
                            }
                        },
                        Qt::QueuedConnection);
            },
            [this]() {
                QMetaObject::invokeMethod(
                        this,
                        [this]() { emit pairingIosNativeQrOverlayBackRequested(); },
                        Qt::QueuedConnection);
            },
            titleUtf8, subtitleUtf8);
#else
    Q_UNUSED(title);
    Q_UNUSED(subtitle);
    qInfo() << "[PairingUi] presentIosPairingQrNativeOverlayScanner: no-op (not iOS build)";
#endif
}

void PairingUiController::dismissIosPairingQrNativeOverlayScanner()
{
#if defined(Q_OS_IOS)
    qInfo() << "[PairingUi] dismissIosPairingQrNativeOverlayScanner";
    amneziaIosPairingQrOverlayDismiss();
#endif
}

void PairingUiController::restartIosPairingQrNativeOverlayCapture()
{
#if defined(Q_OS_IOS)
    qInfo() << "[PairingUi] restartIosPairingQrNativeOverlayCapture";
    amneziaIosPairingQrOverlayRestartCapture();
#endif
}

bool PairingUiController::applyScannedTextAsPairingUuid(const QString &raw)
{
    const QString t = raw.trimmed();
    qInfo() << "[PairingUi] scan raw len=" << t.size();
    const QString uuid = extractPairingSessionUuidFromScanText(raw);
    if (uuid.isEmpty()) {
        if (t.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
            qInfo() << "[PairingUi] scan rejected: looks like vpn:// bundle, not session UUID";
        } else {
            qInfo() << "[PairingUi] scan rejected: no session UUID recognized in payload";
        }
        return false;
    }
    qInfo() << "[PairingUi] scan accepted uuid=" << uuid.left(13) << "...";
    emit pairingUuidFromScan(uuid);
    return true;
}

#if defined(Q_OS_ANDROID)
bool PairingUiController::tryConsumeAndroidQrScan(const QString &code)
{
    if (!g_pairingUiForAndroidQr) {
        qWarning() << "[PairingUi] tryConsumeAndroidQrScan: no controller (g_pairingUiForAndroidQr null)";
        return false;
    }
    const QString codeCopy = code;
    // Parse on this thread: while CameraActivity is foreground, AmneziaActivity is stopped and the Qt
    // event loop may not process BlockingQueuedConnection until the user returns — UI would lag behind.
    if (extractPairingSessionUuidFromScanText(codeCopy).isEmpty()) {
        return false;
    }
    PairingUiController *const ctl = g_pairingUiForAndroidQr;
    QPointer<PairingUiController> ctlPtr(ctl);
    QTimer::singleShot(0, ctl, [ctlPtr, codeCopy]() {
        if (!ctlPtr) {
            return;
        }
        ctlPtr->applyScannedTextAsPairingUuid(codeCopy);
    });
    qInfo() << "[PairingUi] tryConsumeAndroidQrScan: scheduled apply on Qt thread, rawLen=" << codeCopy.size();
    return true;
}

void PairingUiController::notifyAndroidPairingQrCameraClosed()
{
    if (g_pairingUiForAndroidQr) {
        g_pairingUiForAndroidQr->suppressAndroidNativePairingReaderStarts(2000);
    }
}

void PairingUiController::notifyAndroidPairingQrCameraUserDismissed()
{
    if (!g_pairingUiForAndroidQr) {
        return;
    }
    PairingUiController *const ctl = g_pairingUiForAndroidQr;
    QPointer<PairingUiController> ptr(ctl);
    QTimer::singleShot(0, ctl, [ptr]() {
        if (!ptr) {
            return;
        }
        emit ptr->pairingAndroidNativeQrScannerUserDismissed();
    });
}
#endif

void PairingUiController::suppressAndroidNativePairingReaderStarts(int ms)
{
    if (ms <= 0) {
        return;
    }
#if defined(Q_OS_ANDROID)
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 until = now + ms;
    if (until <= m_androidPairingReaderCooldownUntilEpochMs) {
        return;
    }
    m_androidPairingReaderCooldownUntilEpochMs = until;
    emit androidPairingReaderCooldownUntilEpochMsChanged();
#else
    Q_UNUSED(ms);
#endif
}

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

bool PairingUiController::canOpenTvQrPairingPage()
{
    if (!m_appSettingsRepository) {
        emit errorOccurred(ErrorCode::InternalError);
        return false;
    }

    const bool isTestPurchase = false;
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                        m_appSettingsRepository->isDevGatewayEnv(isTestPurchase), kGatewayProbeTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    QByteArray responseBody;
    const ErrorCode err = gatewayController.post(QString::fromLatin1(kGatewayProbePath), QJsonObject {}, responseBody);
    if (err != ErrorCode::NoError) {
        emit errorOccurred(err);
        return false;
    }
    return true;
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
        return tr("QR session expired. A new QR code will appear automatically when the screen refreshes.");
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

    clearPendingPhonePairingUuid();
    if (!m_lastSuccessfulPhonePairingDisplayName.isEmpty()) {
        m_lastSuccessfulPhonePairingDisplayName.clear();
        emit lastSuccessfulPhonePairingDisplayNameChanged();
    }

    cancelTvQrSession();

    setEmbeddedPairingQrCameraActive(false);
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

    if (!m_lastSuccessfulPhonePairingDisplayName.isEmpty()) {
        m_lastSuccessfulPhonePairingDisplayName.clear();
        emit lastSuccessfulPhonePairingDisplayNameChanged();
    }

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
                         QString scanDisplayName;
                         if (logicalErr == ErrorCode::NoError) {
                             logicalErr = PairingController::parseScanQrResponseBody(result.second, &scanDisplayName);
                         }

                         if (logicalErr == ErrorCode::NoError) {
                             setPhoneBusy(false);
                             m_phoneStatusMessage = tr("Sent successfully");
                             emit phoneStatusMessageChanged();
                             if (m_lastSuccessfulPhonePairingDisplayName != scanDisplayName) {
                                 m_lastSuccessfulPhonePairingDisplayName = scanDisplayName;
                                 emit lastSuccessfulPhonePairingDisplayNameChanged();
                             }
                             clearPendingPhonePairingUuid();
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
