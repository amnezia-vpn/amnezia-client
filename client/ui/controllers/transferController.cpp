#include "transferController.h"

#include <QVariant>
#include <QJsonParseError>
#include <QDebug>
#include <qeventloop.h>
#include "core/api/apiUtils.h"

#include "amnezia_application.h"
#include "settings.h"
#include "ui/models/servers_model.h"
#include "ui/controllers/exportController.h"
#include "ui/controllers/importController.h"
#include "core/api/apiDefs.h"
#include "core/controllers/gatewayController.h"

static ErrorCode postPlainJson(const QString& url,
                               const QJsonObject& payload,
                               int timeoutMs,
                               QByteArray& responseBody);

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

QString TransferController::buildQrPayloadJson(const QString &gatewayUrl, const QString &uuid, int version) const
{
    QJsonObject obj;
    obj["gw"] = gatewayUrl;
    obj["u"] = uuid;
    obj["v"] = version;
    qDebug() << "built QrPayload with GW = " << gatewayUrl
             << " uuid = " << uuid
             << " version = " << version;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TransferController::generateNewQrCode()
{
    qDebug() << "TransferController::generateNewQrCode: generating QR code";

    QString gw = m_settings->getGatewayEndpoint();
    if (!gw.endsWith('/')) {
        gw.append('/');
    }
    qDebug() << "gateway:" << gw;
    m_currentUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    qDebug() << "uuid:" << m_currentUuid;

    const QString payload = buildQrPayloadJson(gw, m_currentUuid, 1);

    auto qr = qrCodeUtils::generateQrCode(payload.toUtf8());
    const QString svg = QString::fromStdString(toSvgString(qr, 1));
    m_qrCodeUrl = qrCodeUtils::svgToBase64(svg);

    emit qrCodeUpdated();
}

void TransferController::stopScanner()
{
    qDebug() << "TransferController::stopScanner: emitting scannerShouldStop";
    emit scannerShouldStop();
}

QString TransferController::getPremiumConfigToSend() const
{
    qDebug() << "TransferController:getPremiumConfigToSend() called with apiKey " << apiDefs::key::apiKey;
    //Q_UNUSED(apiDefs::key::apiKey)
    return m_exportController ? m_exportController->getConfig() : QString();
}

QString TransferController::getCurrentApiKey() const
{
    const int idx = m_serversModel ? m_serversModel->getProcessedServerIndex() : -1;
    if (idx < 0 || !m_serversModel) {
        return QString();
    }

    const QJsonObject server = m_serversModel->getServerConfig(idx);

    qDebug() << "server:" << server;

    const QJsonObject apiConfig = server.value(apiDefs::key::apiConfig).toObject();
    QJsonObject authData = server.value(QStringLiteral("auth_data")).toObject();
    //QString key = authData.value(apiDefs::key::apiKey).toString();
    QString key = authData.value(QStringLiteral("api_key")).toString();

    /*if (key.isEmpty()) {
        const QJsonObject nestedAuth = apiConfig.value(QStringLiteral("auth_data")).toObject();
        if (!nestedAuth.isEmpty()) {
            key = nestedAuth.value(apiDefs::key::apiKey).toString();
        }
    }*/

    return key;
}

void TransferController::onTransferQrScanned(const QString &code)
{
    qDebug() << "TransferController  has scanned the Qr";

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(code.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "TransferController::onTransferQrScanned: invalid QR JSON " << err.errorString();
        emit postFailed(QStringLiteral("Invalid QR JSON"));
        return;
    }

    const QJsonObject obj = doc.object();
    QString gw = obj.value("gw").toString();
    const QString uuid = obj.value("u").toString();

    if (gw.isEmpty() || uuid.isEmpty()) {
        qWarning() << "TransferController::onTransferQrScanned: QR missing gw or uuid";
        emit postFailed(QStringLiteral("QR missing gw or uuid"));
        return;
    }
    if (!gw.endsWith('/')) {
        gw.append('/');
    }

    const QString apiKey = getCurrentApiKey();
    qDebug() << "scanned apiKey:" << apiKey;
    const QString config = getPremiumConfigToSend();
    qDebug() << "config:" << config;
    if (apiKey.isEmpty() || config.isEmpty()) {
        qWarning() << "TransferController::onTransferQrScanned: no subscription key or config to send";
        emit postFailed(QStringLiteral("No subscription key or config to send"));
        return;
    }

    // Allow only premium (subscription) configs
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

    qDebug() << "entered POST section";
    qDebug() << "gw:" << gw;
    qDebug() << "uuid:" << uuid;
    qDebug() << "apiKey:" << apiKey;
    qDebug() << "config:" << config;

    /*GatewayController gatewayController(gw, //m_settings->getGatewayEndpoint(),
                                        m_settings->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs,
                                        m_settings->isStrictKillSwitchEnabled());*/
    QByteArray responseBody;


    QJsonObject payload;
    payload.insert(QStringLiteral("config"), config);
    payload.insert(QStringLiteral("uuid"), uuid);
    payload.insert(QStringLiteral("api_key"), apiKey);

    const QString url = gw + "sendConfig";
    qDebug() << "TransferController::onTransferQrScanned: sending POST to" << url
             << "with payload:" << QJsonDocument(payload).toJson(QJsonDocument::Compact);

    auto ec = postPlainJson(url, payload, apiDefs::requestTimeoutMsecs, responseBody);
    qDebug() << "TransferController::onTransferQrScanned: POST finished with code"
             << static_cast<int>(ec);

    if (ec != ErrorCode::NoError) {
        qWarning() << "TransferController::onTransferQrScanned: network error during POST"
                   << "body:" << responseBody;
        emit postFailed(QStringLiteral("Network error"));
        return;
    }

    // Parse response and handle success/failure
    {
        QJsonParseError parseErr;
        const QJsonDocument respDoc = QJsonDocument::fromJson(responseBody, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && respDoc.isObject()) {
            const QJsonObject respObj = respDoc.object();
            const QString status = respObj.value(QStringLiteral("status")).toString();
            if (status == QStringLiteral("success")) {
                qDebug() << "TransferController::onTransferQrScanned: gateway returned success";
                emit postSucceeded();
                stopScanner();
                return;
            }
        }
        qWarning() << "TransferController::onTransferQrScanned: gateway response error" << responseBody;
        emit postFailed(QStringLiteral("Gateway response error"));
        return;
    }
    //const QString endpoint = QStringLiteral("%1sendConfig").arg(gw);
    //const QString endpoint = QStringLiteral("sendConfig");

    /*qDebug() << "TransferController::onTransferQrScanned: sending POST to " << endpoint
            << "with payload: " << QJsonDocument(payload).toJson(QJsonDocument::Compact);
    auto errorCode = gatewayController.post(endpoint, payload, responseBody);
    qDebug() << "TransferController::onTransferQrScanned: POST finished with code"
            << static_cast<int>(errorCode);

    if (errorCode == ErrorCode::NoError) {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(responseBody, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject obj = doc.object();
            if (obj.value(QStringLiteral("status")).toString() == QStringLiteral("success")) {
                qDebug() << "TransferController::onTransferQrScanned: gateway returned success";
                emit postSucceeded();
                stopScanner();
                return;
            }
        }

        qWarning() << "TransferController::onTransferQrScanned: gateway response error";
        emit postFailed(QStringLiteral("Gateway response error"));
    } else {
        qWarning() << "TransferController::onTransferQrScanned: network error during POST";
        emit postFailed(QStringLiteral("Network error"));
    }*/
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
        connect(m_importController, &QObject::destroyed,
                this,
                &TransferController::handleImportControllerDestroyed,
                Qt::UniqueConnection);
    }

    // Blocking request to /waitConfig with a timeout.
    const int waitTimeoutMs = 30000;

    /*GatewayController gatewayController(gw, //m_settings->getGatewayEndpoint(),
                                        m_settings->isDevGatewayEnv(),
                                        waitTimeoutMs,
                                        m_settings->isStrictKillSwitchEnabled());
    const QString endpoint = QStringLiteral("%1waitConfig");
    //const QString endpoint = QStringLiteral("waitConfig");

    qDebug() << "waitConfig endpoint:" << QString(endpoint).arg(gw);*/

    QJsonObject payload;
    payload.insert(QStringLiteral("uuid"), uuid);
    QByteArray responseBody;

    const QString url = gw + "waitConfig";
    qDebug() << "waitConfig endpoint: " << url;
    auto ec = postPlainJson(url, payload, waitTimeoutMs, responseBody);
    if (ec != ErrorCode::NoError) {
        qWarning() << "waitConfig failed, code:" << (int)ec << "body:" << responseBody;
        emit waitError(QStringLiteral("Network error"));
        return;
    }

    /*auto errorCode = gatewayController.post(endpoint, payload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        qWarning() << "TransferController::startWaitForConfig: network error during waitConfig";
        emit waitError(QStringLiteral("Network error"));
        return;
    }*/

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(responseBody, &err);
    if (err.error != QJsonParseError::NoError ||!doc.isObject()) {
        qWarning() << "TransferController::startWaitForConfig: invalid gateway JSON";
        emit waitError(QStringLiteral("Gateway Response Error"));
        return;
    }

    const QJsonObject obj = doc.object();
    QString cfg = obj.value(QStringLiteral("config")).toString();

    if (cfg.isEmpty() || cfg == QStringLiteral("timeout")) {
        qWarning() << "TransferController::startWaitForConfig: timeout or empty config";
        emit waitError(QStringLiteral("Gateway response error (timeout)"));
        return;
    }

    if (!m_importController) {
        qWarning() << "TransferController::startWaitForConfig: import controller is null";
        emit waitError(QStringLiteral("Import Controller destroyed"));
        return;
    }

    if (!m_importController->extractConfigFromData(cfg)) {
        qWarning() << "TransferController::startWaitForConfig: invalid config received from gateway";
        emit waitError(QStringLiteral("Invalid config received from gateway"));
        return;
    }

    m_importController->importConfig();
    emit configApplied();
}

void TransferController::stopWaitForConfig()
{
    qDebug() << "TransferController::stopWaitForConfig: stop flag set";
}

static ErrorCode postPlainJson(const QString& url, const QJsonObject& payload, int timeoutMs, QByteArray& responseBody)
{
    QNetworkRequest request;
    request.setTransferTimeout(timeoutMs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(QUrl(url));

    QNetworkReply* reply = amnApp->networkManager()->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    QEventLoop wait;
    QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);

    QList<QSslError> sslErrors;
    QObject::connect(reply, &QNetworkReply::sslErrors, [&sslErrors](const QList<QSslError>& errors){ sslErrors = errors; });

    wait.exec();
    responseBody = reply->readAll();

    auto ec = apiUtils::checkNetworkReplyErrors(sslErrors, reply);
    reply->deleteLater();
    return ec;
}
