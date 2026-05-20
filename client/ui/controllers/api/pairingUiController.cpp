#include "pairingUiController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
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
#include "core/models/api/apiV2ServerConfig.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/qrCodeUtils.h"

using namespace amnezia;

namespace
{
constexpr auto kGenerateQrPath = "%1v1/generate_qr";
constexpr auto kScanQrPath = "%1v1/scan_qr";
constexpr auto kGatewayProbePath = "%1v1/news";
constexpr int kPairingRetryMaxAttempts = 3;
constexpr int kGatewayProbeTimeoutMsecs = 3000;

QJsonObject apiGatewayServicesFromServers(const ServersController *serversController)
{
    if (!serversController || serversController->getServersCount() == 0) {
        return {};
    }

    QSet<QString> userCountryCodes;
    QSet<QString> serviceTypes;
    for (int i = 0; i < serversController->getServersCount(); ++i) {
        const QString serverId = serversController->getServerId(i);
        const auto apiV2 = serversController->apiV2Config(serverId);
        if (!apiV2.has_value()) {
            continue;
        }
        if (!apiV2->apiConfig.userCountryCode.isEmpty()) {
            userCountryCodes.insert(apiV2->apiConfig.userCountryCode);
        }
        const QString serviceType = apiV2->serviceType();
        if (!serviceType.isEmpty()) {
            serviceTypes.insert(serviceType);
        }
    }

    if (userCountryCodes.isEmpty() && serviceTypes.isEmpty()) {
        return {};
    }

    QJsonObject json;
    QJsonArray userCountryCodesArray;
    for (const QString &code : userCountryCodes) {
        userCountryCodesArray.append(code);
    }
    json.insert(apiDefs::key::userCountryCode, userCountryCodesArray);

    QJsonArray serviceTypesArray;
    for (const QString &type : serviceTypes) {
        serviceTypesArray.append(type);
    }
    json.insert(apiDefs::key::serviceType, serviceTypesArray);
    return json;
}

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

void PairingUiController::openPairingQrScanner()
{
#if defined(Q_OS_ANDROID)
    AndroidController::instance()->startPairingQrReaderActivity();
#endif
}

bool PairingUiController::isPairingCameraAccessGranted() const
{
#if defined(Q_OS_ANDROID)
    return AndroidController::instance()->isCameraPermissionGranted();
#elif defined(Q_OS_IOS)
    return amneziaIosPairingCameraAccessGranted();
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

void PairingUiController::setPairingQrTorchEnabled(bool enabled)
{
#if defined(Q_OS_ANDROID)
    Q_UNUSED(enabled);
#elif defined(Q_OS_IOS)
    amneziaIosPairingQrOverlaySetTorchEnabled(enabled);
#else
    Q_UNUSED(enabled);
#endif
}

void PairingUiController::presentIosPairingQrNativeOverlayScanner(const QString &title, const QString &subtitle)
{
#if defined(Q_OS_IOS)
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
#endif
}

void PairingUiController::dismissIosPairingQrNativeOverlayScanner()
{
#if defined(Q_OS_IOS)
    amneziaIosPairingQrOverlayDismiss();
#endif
}

void PairingUiController::restartIosPairingQrNativeOverlayCapture()
{
#if defined(Q_OS_IOS)
    amneziaIosPairingQrOverlayRestartCapture();
#endif
}

bool PairingUiController::applyScannedTextAsPairingUuid(const QString &raw)
{
    const QString uuid = extractPairingSessionUuidFromScanText(raw);
    if (uuid.isEmpty()) {
        return false;
    }

    emit pairingUuidFromScan(uuid);
    return true;
}

#if defined(Q_OS_ANDROID)
bool PairingUiController::tryConsumeAndroidQrScan(const QString &code)
{
    if (!g_pairingUiForAndroidQr) {
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

void PairingUiController::setTvBusy(bool busy)
{
    m_tvPairingBusy = busy;
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

    const QJsonObject gatewayServices = apiGatewayServicesFromServers(m_serversController);
    if (gatewayServices.isEmpty()) {
        return true;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("locale"), m_appSettingsRepository->getAppLanguage().name().split(QLatin1Char('_')).first());

    if (gatewayServices.contains(apiDefs::key::userCountryCode)) {
        payload.insert(apiDefs::key::userCountryCode, gatewayServices.value(apiDefs::key::userCountryCode));
    }
    if (gatewayServices.contains(apiDefs::key::serviceType)) {
        payload.insert(apiDefs::key::serviceType, gatewayServices.value(apiDefs::key::serviceType));
    }

    const bool isTestPurchase = false;
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                        m_appSettingsRepository->isDevGatewayEnv(isTestPurchase), kGatewayProbeTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    QByteArray responseBody;
    const ErrorCode err = gatewayController.post(QString::fromLatin1(kGatewayProbePath), payload, responseBody);
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

    setTvBusy(true);

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
            gatewayController->postAsync(QString::fromLatin1(kGenerateQrPath), payload, &replyRaw, gatewayController);
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
                             const ErrorCode impErr = m_subscriptionController->importServerFromQrPairingResponse(
                                     out.config, out.serviceInfo, out.supportedProtocols);
                             setTvBusy(false);
                             if (impErr != ErrorCode::NoError) {
                                 emit errorOccurred(impErr);
                                 resetTvQrDisplay();
                                 return;
                             }
                             resetTvQrDisplay();
                             emit tvPairingConfigReceived();
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
    resetTvQrDisplay();
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

    clearPendingPhonePairingUuid();
    if (!m_lastSuccessfulPhonePairingDisplayName.isEmpty()) {
        m_lastSuccessfulPhonePairingDisplayName.clear();
        emit lastSuccessfulPhonePairingDisplayNameChanged();
    }

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
    if (trimmedUuid.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return;
    }

    if (serverIndex < 0 || serverIndex >= m_serversController->getServersCount()) {
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    const QString serverId = m_serversController->getServerId(serverIndex);
    const auto apiV2Opt = m_serversController->apiV2Config(serverId);
    if (!apiV2Opt.has_value()) {
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    const ApiV2ServerConfig &apiV2 = *apiV2Opt;

    QString vpnKey;
    const ErrorCode keyErr = m_subscriptionController->prepareVpnKeyExport(serverId, vpnKey);
    if (keyErr != ErrorCode::NoError) {
        emit errorOccurred(keyErr);
        return;
    }

    const QJsonObject serviceInfo = apiV2.apiConfig.serviceInfo.toJson();
    const QJsonArray supportedProtocols = apiV2.apiConfig.supportedProtocols;
    const QString apiKey = apiV2.authData.apiKey;
    if (apiKey.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return;
    }

    const QString serviceType = apiV2.apiConfig.serviceType.trimmed();
    const QString userCountryCode = apiV2.apiConfig.userCountryCode.trimmed();

    const ErrorCode fieldErr =
            PairingController::validatePairingScanFields(trimmedUuid, vpnKey, apiKey, serviceType, userCountryCode);
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

    setPhoneBusy(true);

    dispatchPhoneScanQrAttempt(trimmedUuid, apiV2.apiConfig.isTestPurchase, vpnKey, serviceInfo, supportedProtocols, apiKey,
                               serviceType, userCountryCode, phoneGeneration, 0);
}

void PairingUiController::dispatchPhoneScanQrAttempt(const QString &qrUuid, const bool isTestPurchase, const QString &vpnKey,
                                                     const QJsonObject &serviceInfo, const QJsonArray &supportedProtocols,
                                                     const QString &apiKey, const QString &serviceType, const QString &userCountryCode,
                                                     quint64 generation, int retryAttempt)
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

    const QJsonObject payload = m_pairingController->buildScanQrPayload(qrUuid, vpnKey, serviceInfo, supportedProtocols, apiKey,
                                                                        serviceType, userCountryCode);
    QNetworkReply *replyRaw = nullptr;
    const QFuture<QPair<ErrorCode, QByteArray>> future =
            gatewayController->postAsync(QString::fromLatin1(kScanQrPath), payload, &replyRaw, gatewayController);
    m_phoneNetworkReply = replyRaw;

    auto *watcher = new QFutureWatcher<QPair<ErrorCode, QByteArray>>(this);
    m_phoneWatcher = watcher;
    QObject::connect(watcher, &QFutureWatcher<QPair<ErrorCode, QByteArray>>::finished, this,
                     [this, gatewayController, watcher, generation, retryAttempt, qrUuid, isTestPurchase, vpnKey, serviceInfo,
                      supportedProtocols, apiKey, serviceType, userCountryCode]() {
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
                                                             apiKey, serviceType, userCountryCode, generation, retryAttempt]() {
                                if (generation != m_phoneSessionGeneration) {
                                    return;
                                }
                                dispatchPhoneScanQrAttempt(qrUuid, isTestPurchase, vpnKey, serviceInfo, supportedProtocols, apiKey,
                                                           serviceType, userCountryCode, generation, retryAttempt + 1);
                            });
                             return;
                         }

                         setPhoneBusy(false);
                         emit errorOccurred(logicalErr);
                     });
    watcher->setFuture(future);
}
