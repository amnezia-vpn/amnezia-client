#include "pairingUiController.h"

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
    AndroidController::instance()->startQrReaderActivity();
#endif
}

bool PairingUiController::applyScannedTextAsPairingUuid(const QString &raw)
{
    const QString t = raw.trimmed();
    if (t.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
        return false;
    }
    static const QRegularExpression re(QStringLiteral(
            "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}"));
    const QRegularExpressionMatch m = re.match(t);
    if (!m.hasMatch()) {
        return false;
    }
    const QString uuid = m.captured(0);
    emit pairingUuidFromScan(uuid);
    return true;
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
    m_tvQrCodes = qrCodeUtils::generateQrCodeImageSeries(qrPayload);
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
    if (trimmedUuid.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return;
    }

    if (serverIndex < 0 || serverIndex >= m_serversController->getServersCount()) {
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    const ServerConfig serverConfig = m_serversController->getServerConfig(serverIndex);
    if (!serverConfig.isApiV2()) {
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    const ApiV2ServerConfig *apiV2 = serverConfig.as<ApiV2ServerConfig>();
    if (!apiV2) {
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    QString vpnKey;
    const ErrorCode keyErr = m_subscriptionController->prepareVpnKeyExport(serverIndex, vpnKey);
    if (keyErr != ErrorCode::NoError) {
        emit errorOccurred(keyErr);
        return;
    }

    const QJsonObject serviceInfo = apiV2->apiConfig.serviceInfo.toJson();
    const QJsonArray supportedProtocols = apiV2->apiConfig.supportedProtocols;
    const QString apiKey = apiV2->authData.apiKey;
    if (apiKey.isEmpty()) {
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
