#include "transferController.h"

#include <QVariant>
#include <QJsonParseError>
#include <QDebug>
#include <qeventloop.h>
#include <QNetworkProxyFactory>
#include <QNetworkProxyQuery>
#include <QUrl>
#include "core/api/apiUtils.h"

#include "amnezia_application.h"
#include "settings.h"
#include "ui/models/servers_model.h"
#include "ui/controllers/exportController.h"
#include "ui/controllers/importController.h"
#include "core/api/apiDefs.h"
#include "core/controllers/gatewayController.h"
#include "core/errorstrings.h"

namespace {
    void logSystemProxiesForUrl(const QString &urlStr)
    {
        const QUrl url(urlStr);
        const QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(QNetworkProxyQuery(url));
        QStringList proxyDesc;
        proxyDesc.reserve(proxies.size());
        for (const auto &p : proxies) {
            proxyDesc << QStringLiteral("%1 %2:%3")
                                 .arg(p.type() == QNetworkProxy::NoProxy ? QStringLiteral("NoProxy")
                                      : p.type() == QNetworkProxy::HttpProxy ? QStringLiteral("HttpProxy")
                                      : p.type() == QNetworkProxy::Socks5Proxy ? QStringLiteral("Socks5Proxy")
                                      : QStringLiteral("Proxy"))
                                 .arg(p.hostName())
                                 .arg(p.port());
        }
        qDebug() << "TransferController: system proxies for" << urlStr << ":" << proxyDesc;
    }
}

TransferController::TransferController(const std::shared_ptr<Settings> &settings,
                                       const QSharedPointer<ServersModel> &serversModel,
                                       ExportController *exportController,
                                       QObject *parent)
    : QObject(parent), m_settings(settings), m_serversModel(serversModel), m_exportController(exportController)
{
    qDebug() << "TransferController created";
}

void TransferController::handleImportControllerDestroyed()
{
    m_importController = nullptr;
    stopWaitForConfig();
}

TransferController::~TransferController() {
}

QString TransferController::buildQrPayloadJson(const QString &gatewayUrl, const QString &uuid) const
{
    QJsonObject obj;
    obj["gw"] = gatewayUrl;
    obj["uuid"] = uuid;
    qDebug() << "built QrPayload with GW = " << gatewayUrl
             << " uuid = " << uuid;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TransferController::generateNewQrCode()
{
    // Debug mode: keep UUID/payload generation, but disable actual QR rendering (temporary).
    qDebug() << "TransferController::generateNewQrCode: generating transfer payload (QR rendering disabled)";

    QString gw = m_settings->getGatewayEndpoint();
    if (!gw.endsWith('/')) {
        gw.append('/');
    }
    qDebug() << "gateway:" << gw;
    m_currentUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    qDebug() << "uuid:" << m_currentUuid;

    m_currentPayload = buildQrPayloadJson(gw, m_currentUuid);

    // QR generation disabled for debugging/CLI-style copy-paste flow.
    // If/when QR is re-enabled, restore this block:
    //
    // auto qr = qrCodeUtils::generateQrCode(m_currentPayload.toUtf8());
    // const QString svg = QString::fromStdString(toSvgString(qr, 1));
    // m_qrCodeUrl = qrCodeUtils::svgToBase64(svg);
    // emit qrCodeUpdated();

    m_qrCodeUrl.clear();
    emit qrCodeUpdated();
    emit currentUuidChanged();
    emit currentPayloadChanged();
}

void TransferController::stopScanner()
{
    qDebug() << "TransferController::stopScanner: emitting scannerShouldStop";
    emit scannerShouldStop();
}

QString TransferController::getCurrentApiKey(QString *vpnKeyOut) const
{
    const int idx = m_serversModel ? m_serversModel->getProcessedServerIndex() : -1;
    if (idx < 0 || !m_serversModel) {
        return QString();
    }

    const QJsonObject server = m_serversModel->getServerConfig(idx);

    qDebug() << "server:" << server;

    const QJsonObject apiConfig = server.value(apiDefs::key::apiConfig).toObject();
    const QJsonObject authData = server.value(QStringLiteral("auth_data")).toObject();

    const QString apiKey = authData.value(QStringLiteral("api_key")).toString();

    if (vpnKeyOut) {
        QString vpnKey = apiConfig.value(apiDefs::key::vpnKey).toString();
        if (vpnKey.isEmpty()) {
            // Fallback for older Premium V1 configs where vpn_key may be derived.
            vpnKey = apiUtils::getPremiumV1VpnKey(server);
        }
        *vpnKeyOut = vpnKey;
    }

    return apiKey;
}

void TransferController::onTransferQrScanned(const QString &code)
{
    qDebug() << "TransferController has scanned the Qr";

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(code.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "TransferController::onTransferQrScanned: invalid QR JSON " << err.errorString();
        emit postFailed(QStringLiteral("Invalid QR JSON"));
        return;
    }

    const QJsonObject obj = doc.object();
    QString gw = obj.value("gw").toString();
    const QString uuid = obj.value("uuid").toString();

    if (gw.isEmpty() || uuid.isEmpty()) {
        qWarning() << "TransferController::onTransferQrScanned: QR missing gw or uuid";
        emit postFailed(QStringLiteral("QR missing gw or uuid"));
        return;
    }
    if (!gw.endsWith('/')) {
        gw.append('/');
    }

    QString vpnKey;
    const QString apiKey = getCurrentApiKey(&vpnKey);
    qDebug() << "scanned apiKey:" << apiKey;
    if (apiKey.isEmpty()) {
        qWarning() << "TransferController::onTransferQrScanned: no subscription key or config to send";
        emit postFailed(QStringLiteral("No subscription key or config to send"));
        return;
    }

    bool isPremium = m_serversModel && m_serversModel->processedServerIsPremium();
    qDebug() << "isPremium: " << isPremium;
    bool isFromGatewayApi = m_serversModel && m_serversModel->getProcessedServerData("isServerFromGatewayApi").toBool();
    qDebug() << "is from gatewayApi: " << isFromGatewayApi;
    if (!isPremium && !isFromGatewayApi) {
        qWarning() << "TransferController::onTransferQrScanned: premium subscription required";
        emit postFailed(QStringLiteral("Premium subscription required"));
        return;
    }

    emit postStarted();

    if (vpnKey.isEmpty()) {
        qWarning() << "TransferController::onTransferQrScanned: missing vpn_key";
        emit postFailed(QStringLiteral("Missing vpn_key to send"));
        return;
    }

    // sendConfig can take longer on slower networks / debug setups, so use a bigger timeout than generic API calls.
    const int sendTimeoutMs = 120000;
    GatewayController gatewayController(gw,
                                        m_settings->isDevGatewayEnv(),
                                        sendTimeoutMs,
                                        m_settings->isStrictKillSwitchEnabled());

    QJsonObject payload;
    payload.insert(QStringLiteral("uuid"), uuid);
    payload.insert(QStringLiteral("api_key"), apiKey);
    payload.insert(QStringLiteral("config"), vpnKey);

    const QString endpoint = QStringLiteral("%1v1/sendConfig");
    QByteArray responseBody;
    const QString fullUrl = endpoint.arg(gw);
    qDebug() << "TransferController::onTransferQrScanned: POST" << fullUrl
             << "uuid:" << uuid;
    logSystemProxiesForUrl(fullUrl);
    const auto errorCode = gatewayController.post(endpoint, payload, responseBody);
    qDebug() << "TransferController::onTransferQrScanned: sendConfig finished with code"
             << static_cast<int>(errorCode)
             << "response size:" << responseBody.size();

    if (errorCode != ErrorCode::NoError) {
        qWarning() << "TransferController::onTransferQrScanned: sendConfig failed with code" << static_cast<int>(errorCode);
        emit postFailed(QStringLiteral("sendConfig failed: %1").arg(errorString(errorCode)));
        return;
    }

    QJsonParseError parseErr;
    const QJsonDocument respDoc = QJsonDocument::fromJson(responseBody, &parseErr);
    if (parseErr.error == QJsonParseError::NoError && respDoc.isObject()
        && respDoc.object().value(QStringLiteral("status")).toString() == QStringLiteral("success")) {
        emit postSucceeded();
        stopScanner();
        return;
    }

    qWarning() << "TransferController::onTransferQrScanned: unexpected gateway response:" << responseBody;
    emit postFailed(QStringLiteral("Gateway response error"));
}

QString TransferController::qrCodeUrl() const
{
    return m_qrCodeUrl;
}

void TransferController::startWaitForConfig(ImportController *importController)
{
    QString gw = m_settings->getGatewayEndpoint();
    if (!gw.endsWith(QLatin1Char('/'))) {
        gw.append(QLatin1Char('/'));
    }

    const QString uuid = m_currentUuid;
    qDebug() << "TransferController::startWaitForConfig: starting blocking wait with uuid: " << uuid;

    if (uuid.isEmpty()) {
        qWarning() << "TransferController::startWaitForConfig: no uuid";
        emit waitError(QStringLiteral("No UUID"));
        return;
    }

    m_importController = importController;
    if (m_importController) {
        connect(m_importController, &ImportController::destroyed,
                this,
                &TransferController::handleImportControllerDestroyed,
                Qt::UniqueConnection);
    }

    const int waitTimeoutMs = 300000;

    QJsonObject payload;
    payload.insert(QStringLiteral("uuid"), uuid);

    GatewayController gatewayController(gw,
                                        m_settings->isDevGatewayEnv(),
                                        waitTimeoutMs,
                                        m_settings->isStrictKillSwitchEnabled());

    const QString endpoint = QStringLiteral("%1v1/waitConfig");
    QByteArray responseBody;
    const QString fullUrl = endpoint.arg(gw);
    qDebug() << "TransferController::startWaitForConfig: POST" << fullUrl
             << "uuid:" << uuid;
    logSystemProxiesForUrl(fullUrl);
    const auto errorCode = gatewayController.post(endpoint, payload, responseBody);

    if (errorCode != ErrorCode::NoError) {
        qWarning() << "TransferController::startWaitForConfig: waitConfig failed with code" << static_cast<int>(errorCode);
        emit waitError(QStringLiteral("waitConfig failed (%1)").arg(static_cast<int>(errorCode)));
        return;
    }

    if (!m_importController) {
        qWarning() << "TransferController::startWaitForConfig: import controller is null";
        emit waitError(QStringLiteral("Import Controller destroyed"));
        return;
    }

    QJsonParseError parseErr;
    const QJsonDocument respDoc = QJsonDocument::fromJson(responseBody, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !respDoc.isObject()) {
        qWarning() << "TransferController::startWaitForConfig: invalid JSON response:" << responseBody;
        emit waitError(QStringLiteral("Invalid gateway response"));
        return;
    }
    const QJsonObject respObj = respDoc.object();
    const QString status = respObj.value(QStringLiteral("status")).toString();
    const QString configStr = respObj.value(QStringLiteral("config")).toString();
    if (status != QStringLiteral("success")) {
        qWarning() << "TransferController::startWaitForConfig: gateway status not success:" << status;
        emit waitError(QStringLiteral("Gateway error"));
        return;
    }
    if (configStr.isEmpty()) {
        emit waitError(QStringLiteral("Empty config"));
        return;
    }
    if (configStr == QStringLiteral("timeout")) {
        emit waitError(QStringLiteral("Timeout"));
        return;
    }

    if (!m_importController->extractConfigFromData(configStr)) {
        qWarning() << "TransferController::startWaitForConfig: failed to parse config string";
        emit waitError(QStringLiteral("Invalid config payload"));
        return;
    }

    m_importController->importConfig();
    emit configApplied();
}
void TransferController::stopWaitForConfig()
{
    qDebug() << "TransferController::stopWaitForConfig: stop flag set";
}

