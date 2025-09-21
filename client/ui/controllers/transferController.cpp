#include "transferController.h"

#include <QVariant>
#include <QJsonParseError>
#include <QUrlQuery>
#include <QtConcurrent/QtConcurrentRun>
#include <QThread>

#include "settings.h"
#include "ui/models/servers_model.h"
#include "ui/controllers/exportController.h"
#include "ui/controllers/importController.h"
#include "core/api/apiDefs.h"
#include "core/controllers/gatewayController.h"

TransferController::TransferController(const std::shared_ptr<Settings> &settings,
                                       const QSharedPointer<ServersModel> &serversModel,
                                       ExportController *exportController,
                                       QObject *parent)
    : QObject(parent), m_settings(settings), m_serversModel(serversModel), m_exportController(exportController)
{
}

QString TransferController::buildQrPayloadJson(const QString &gatewayUrl, const QString &uuid, int version) const
{
    QJsonObject obj;
    obj["gw"] = gatewayUrl;
    obj["u"] = uuid;
    obj["v"] = version;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TransferController::generateNewQrCode()
{
    QString gw = m_settings->getGatewayEndpoint();
    if (!gw.endsWith('/')) gw.append('/');
    m_currentUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QString payload = buildQrPayloadJson(gw, m_currentUuid, 1);

    auto qr = qrCodeUtils::generateQrCode(payload.toUtf8());
    const QString svg = QString::fromStdString(toSvgString(qr, 1));
    m_qrCodeUrl = qrCodeUtils::svgToBase64(svg);

    // Bump generation so existing wait loops can detect regeneration
    m_waitGeneration.fetchAndAddRelaxed(1);

    emit qrCodeUpdated();
}

void TransferController::stopScanner()
{
    emit scannerShouldStop();
}

QString TransferController::getPremiumConfigToSend() const
{
    Q_UNUSED(apiDefs::key::apiKey)
    return m_exportController ? m_exportController->getConfig() : QString();
}

QString TransferController::getCurrentApiKey() const
{
    const int idx = m_serversModel ? m_serversModel->getProcessedServerIndex() : -1;
    if (idx >= 0 && m_serversModel) {
        const QJsonObject server = m_serversModel->getServerConfig(idx);
        const QJsonObject apiConfig = server.value(apiDefs::key::apiConfig).toObject();
        const QString key = apiConfig.value(apiDefs::key::apiKey).toString();
        return key;
    }
    return QString();
}

void TransferController::onTransferQrScanned(const QString &code)
{
    if (m_postInFlight.loadAcquire()) {
        // prevent duplicate POSTs
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(code.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit postFailed(QStringLiteral("Invalid QR JSON"));
        return;
    }
    const QJsonObject obj = doc.object();
    QString gw = obj.value("gw").toString();
    const QString uuid = obj.value("u").toString();

    if (gw.isEmpty() || uuid.isEmpty()) {
        emit postFailed(QStringLiteral("QR missing gw or uuid"));
        return;
    }
    if (!gw.endsWith('/')) gw.append('/');

    const QString apiKey = getCurrentApiKey();
    const QString config = getPremiumConfigToSend();
    if (apiKey.isEmpty() || config.isEmpty()) {
        emit postFailed(QStringLiteral("No subscription key or config to send"));
        return;
    }
    // Allow only premium (subscription) configs
    bool isPremium = m_serversModel && m_serversModel->processedServerIsPremium();
    bool isFromGatewayApi = m_serversModel && m_serversModel->getProcessedServerData("isServerFromGatewayApi").toBool();
    if (!isPremium && !isFromGatewayApi) {
        emit postFailed(QStringLiteral("Premium subscription required"));
        return;
    }

    m_postInFlight.storeRelease(1);
    emit postStarted();

    m_postFuture = QtConcurrent::run([this, gw, uuid, apiKey, config]() {

        GatewayController gatewayController(m_settings->getGatewayEndpoint(),
                                            m_settings->isDevGatewayEnv(),
                                            apiDefs::requestTimeoutMsecs,
                                            m_settings->isStrictKillSwitchEnabled());
        QByteArray responseBody;
        QJsonObject payload;
        payload.insert("config", config);

        // URL-encode query
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("uuid"), uuid);
        q.addQueryItem(QStringLiteral("api_key"), apiKey);
        const QString endpoint = QString("%1sendConfig?%2").arg(gw, q.query(QUrl::FullyEncoded));

        auto errorCode = gatewayController.post(endpoint, payload, responseBody);

        QMetaObject::invokeMethod(this, [this, errorCode, responseBody]() {
            m_postInFlight.storeRelease(0);
            if (errorCode == ErrorCode::NoError) {
                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(responseBody, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    const QJsonObject obj = doc.object();
                    if (obj.value("status").toString() == QStringLiteral("success")) {

                        emit postSucceeded();
                        stopScanner();
                        return;
                    }
                }
                emit postFailed(QStringLiteral("Gateway response error"));
            } else {
                emit postFailed(QStringLiteral("Network error"));
            }
        }, Qt::QueuedConnection);
    });
}

QString TransferController::qrCodeUrl() const
{
    return m_qrCodeUrl;
}

void TransferController::startWaitForConfig(ImportController *importController)
{
    m_importController = importController;
    m_stopWaiting.storeRelease(0);

    QString gw = m_settings->getGatewayEndpoint();
    if (!gw.endsWith('/')) gw.append('/');
    const QString uuid = m_currentUuid;
    const QString apiKey = getCurrentApiKey();
    const int generation = m_waitGeneration.loadAcquire();



    m_waitFuture = QtConcurrent::run([this, gw, uuid, apiKey, generation]() {
        int backoffMs = 500;
        const int maxBackoffMs = 5000;

        while (!m_stopWaiting.loadAcquire()) {
            if (generation != m_waitGeneration.loadAcquire()) break;

            QByteArray responseBody;
            GatewayController gatewayController(m_settings->getGatewayEndpoint(),
                                                m_settings->isDevGatewayEnv(),
                                                apiDefs::requestTimeoutMsecs,
                                                m_settings->isStrictKillSwitchEnabled());

            QUrlQuery q;
            q.addQueryItem(QStringLiteral("uuid"), uuid);
            q.addQueryItem(QStringLiteral("api_key"), apiKey);
            const QString endpoint = QString("%1waitConfig?%2").arg(gw, q.query(QUrl::FullyEncoded));

            auto errorCode = gatewayController.get(endpoint, responseBody);

            if (m_stopWaiting.loadAcquire()) break;
            if (generation != m_waitGeneration.loadAcquire()) break;

            if (errorCode == ErrorCode::NoError) {
                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(responseBody, &err);
                if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                    emit waitError(QStringLiteral("Gateway response error"));
                } else {
                    const QJsonObject obj = doc.object();
                    const QString cfg = obj.value("config").toString();
                    if (cfg == QStringLiteral("timeout")) {
                        backoffMs = 500;
                        continue;
                    }
                    if (!cfg.isEmpty()) {

                        QMetaObject::invokeMethod(this, [this, cfg]() {
                            if (!m_importController) return;
                            m_importController->extractConfigFromData(cfg);
                            m_importController->importConfig();
                            emit configApplied();
                        }, Qt::QueuedConnection);
                        break;
                    }
                }
            } else {
                emit waitError(QStringLiteral("Network error"));
            }

            QThread::msleep(static_cast<unsigned long>(backoffMs));
            backoffMs = qMin(backoffMs * 2, maxBackoffMs);
        }


    });
}

void TransferController::stopWaitForConfig()
{
    m_stopWaiting.storeRelease(1);

}
