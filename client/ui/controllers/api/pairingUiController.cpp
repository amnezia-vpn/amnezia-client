#include "pairingUiController.h"

#include <QUuid>

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

    m_tvSessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QByteArray qrPayload = m_tvSessionUuid.toUtf8();
    m_tvQrCodes = qrCodeUtils::generateQrCodeImageSeries(qrPayload);
    emit tvQrCodesChanged();
    emit tvSessionUuidChanged();

    m_tvStatusMessage = tr("Waiting for premium device to confirm…");
    emit tvStatusMessageChanged();

    setTvBusy(true);

    const bool isTestPurchase = false;
    auto gatewayController = QSharedPointer<GatewayController>::create(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                                                       m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                                                       m_pairingController->pairingLongPollTimeoutMsecs(),
                                                                       m_appSettingsRepository->isStrictKillSwitchEnabled());

    const QJsonObject payload = m_pairingController->buildGenerateQrPayload(m_tvSessionUuid);
    const QFuture<QPair<ErrorCode, QByteArray>> future = gatewayController->postAsync(QString::fromLatin1(kGenerateQrPath), payload);

    auto *watcher = new QFutureWatcher<QPair<ErrorCode, QByteArray>>(this);
    m_tvWatcher = watcher;
    QObject::connect(watcher, &QFutureWatcher<QPair<ErrorCode, QByteArray>>::finished, this,
                     [this, gatewayController, watcher]() {
                         Q_UNUSED(gatewayController);
                         const auto result = watcher->result();
                         watcher->deleteLater();
                         if (m_tvWatcher == watcher) {
                             m_tvWatcher.clear();
                         }

                         setTvBusy(false);

                         if (result.first != ErrorCode::NoError) {
                             m_tvStatusMessage = tr("Pairing failed");
                             emit tvStatusMessageChanged();
                             emit errorOccurred(result.first);
                             return;
                         }

                         PairingController::QrPairingConfigPayload out;
                         const ErrorCode parseErr = PairingController::parseGenerateQrResponseBody(result.second, out);
                         if (parseErr != ErrorCode::NoError) {
                             m_tvStatusMessage = tr("Pairing failed");
                             emit tvStatusMessageChanged();
                             emit errorOccurred(parseErr);
                             return;
                         }

                         m_tvStatusMessage = tr("Configuration received");
                         emit tvStatusMessageChanged();
                         emit tvPairingConfigReceived();
                     });
    watcher->setFuture(future);
}

void PairingUiController::cancelTvQrSession()
{
    if (m_tvWatcher) {
        m_tvWatcher->disconnect();
        m_tvWatcher->deleteLater();
        m_tvWatcher.clear();
    }
    setTvBusy(false);
    m_tvStatusMessage.clear();
    emit tvStatusMessageChanged();
    resetTvQrDisplay();
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

    m_phoneStatusMessage = tr("Sending…");
    emit phoneStatusMessageChanged();
    setPhoneBusy(true);

    runPhonePairingRequest(trimmedUuid, apiV2->apiConfig.isTestPurchase, vpnKey, serviceInfo, supportedProtocols, apiKey);
}

void PairingUiController::runPhonePairingRequest(const QString &qrUuid, const bool isTestPurchase, const QString &vpnKey,
                                                 const QJsonObject &serviceInfo, const QJsonArray &supportedProtocols,
                                                 const QString &apiKey)
{
    auto gatewayController = QSharedPointer<GatewayController>::create(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                                                       m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                                                       apiDefs::requestTimeoutMsecs,
                                                                       m_appSettingsRepository->isStrictKillSwitchEnabled());

    const QJsonObject payload = m_pairingController->buildScanQrPayload(qrUuid, vpnKey, serviceInfo, supportedProtocols, apiKey);
    const QFuture<QPair<ErrorCode, QByteArray>> future = gatewayController->postAsync(QString::fromLatin1(kScanQrPath), payload);

    auto *watcher = new QFutureWatcher<QPair<ErrorCode, QByteArray>>(this);
    m_phoneWatcher = watcher;
    QObject::connect(watcher, &QFutureWatcher<QPair<ErrorCode, QByteArray>>::finished, this,
                     [this, gatewayController, watcher]() {
                         Q_UNUSED(gatewayController);
                         const auto result = watcher->result();
                         watcher->deleteLater();
                         if (m_phoneWatcher == watcher) {
                             m_phoneWatcher.clear();
                         }

                         setPhoneBusy(false);

                         if (result.first != ErrorCode::NoError) {
                             m_phoneStatusMessage = tr("Send failed");
                             emit phoneStatusMessageChanged();
                             emit errorOccurred(result.first);
                             return;
                         }

                         const ErrorCode parseErr = PairingController::parseScanQrResponseBody(result.second);
                         if (parseErr != ErrorCode::NoError) {
                             m_phoneStatusMessage = tr("Send failed");
                             emit phoneStatusMessageChanged();
                             emit errorOccurred(parseErr);
                             return;
                         }

                         m_phoneStatusMessage = tr("Sent successfully");
                         emit phoneStatusMessageChanged();
                         emit phonePairingSucceeded();
                     });
    watcher->setFuture(future);
}
