#include "transferController.h"

#include <QVariant>
#include <QJsonParseError>
#include <QUrlQuery>
#include <QtConcurrent/QtConcurrentRun>
#include <QThread>
#include <QDebug>

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
    qDebug() << "TransferController created";
}

void TransferController::handleImportControllerDestroyed()
{
    m_importController = nullptr;
    stopWaitForConfig();
}

TransferController::~TransferController() {
    m_stopWaiting.storeRelease(1);
    if (m_waitFuture.isRunning())
        m_waitFuture.waitForFinished();
    if (m_postFuture.isRunning())
        m_postFuture.waitForFinished();
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
    qDebug() << "gateway: " << gw;
    m_currentUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    qDebug() << "uuid: " <<m_currentUuid;

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
    QString key = authData.value(apiDefs::key::apiKey).toString();

    if (key.isEmpty()) {
        const QJsonObject nestedAuth =
                apiConfig.value(QStringLiteral("auth_data")).toObject();
        if (!nestedAuth.isEmpty()) {
            key = nestedAuth.value(apiDefs::key::apiKey).toString();
        }
    }

    return key;
}

void TransferController::onTransferQrScanned(const QString &code)
{
    qDebug() << "TransferController  has scanned the Qr";
    if (m_postInFlight.loadAcquire()) {
        // prevent duplicate POSTs
        return;
    }

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
    qDebug() << "scanned apiKey: " << apiKey;
    const QString config = getPremiumConfigToSend();
    qDebug() << "config: " << config;
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

    m_postInFlight.storeRelease(1);
    emit postStarted();

    m_postFuture = QtConcurrent::run([this, gw, uuid, apiKey, config]() {

        qDebug() << "entered QFuture section";
        qDebug() << "gw: " << gw;
        qDebug() << "uuid: " << uuid;
        qDebug() << "apiKey: " << apiKey;
        qDebug() << "config: " << config;

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
        const QString endpoint = QStringLiteral("%1sendConfig?%2")
                                        .arg(gw)
                                        .arg(q.query(QUrl::FullyEncoded));

        qDebug() << "TransferController::onTransferQrScanned: sending POST to" << endpoint;
        auto errorCode = gatewayController.post(endpoint, payload, responseBody);
        qDebug() << "TransferController::onTransferQrScanned: POST finished with code"
                 << static_cast<int>(errorCode);

        QMetaObject::invokeMethod(this, [this, errorCode, responseBody]() {
            m_postInFlight.storeRelease(0);
            if (errorCode == ErrorCode::NoError) {
                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(responseBody, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    const QJsonObject obj = doc.object();
                    if (obj.value("status").toString() == QStringLiteral("success")) {

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
    if (m_waitFuture.isRunning()) {
        qWarning() << "startWaitForConfig called while previous wait is still running; ignoring";
        return;
    }

    QString gw = m_settings->getGatewayEndpoint();
    if (!gw.endsWith('/')) {
        gw.append('/');
    }

    const QString uuid = m_currentUuid;
    qDebug() << "TransferController::startWaitForConfig: starting wait with uuid" << m_currentUuid;
    const int generation = m_waitGeneration.loadAcquire();

    qDebug() << "gw: " << gw;
    qDebug() << "uuid: " << uuid;
    qDebug() << "generation: " << generation;

    if (uuid.isEmpty()) {
        qDebug() << "error: no uuid";
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

    m_stopWaiting.storeRelease(0);

    m_waitFuture = QtConcurrent::run([this, gw, uuid, generation]() {
        qDebug() << "started waiting for QFuture";
        //int backoffMs = 500;
        //const int maxBackoffMs = 5000;
        const int pollIntervalMs = 6000;

        qDebug() << "TransferController::startWaitForConfig: waiting for config with"
                 << "gw =" << gw
                 << "uuid =" << uuid
                 << "generation =" << generation;

        while (!m_stopWaiting.loadAcquire()) {
            if (generation != m_waitGeneration.loadAcquire()) {
                break;
            }

            QUrlQuery q;
            q.addQueryItem(QStringLiteral("uuid"), uuid);

            const QString endpoint = QStringLiteral("%1waitConfig?%2")
                                             .arg(gw)
                                             .arg(q.query(QUrl::FullyEncoded));
            qDebug() << "waitConfig endpoint:" << endpoint;

            QByteArray responseBody;
            GatewayController gatewayController(m_settings->getGatewayEndpoint(),
                                                m_settings->isDevGatewayEnv(),
                                                apiDefs::requestTimeoutMsecs,
                                                m_settings->isStrictKillSwitchEnabled());

            auto errorCode = gatewayController.get(endpoint, responseBody);
            if (errorCode == ErrorCode::NoError) {
                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(responseBody, &err);
                if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                    emit waitError(QStringLiteral("Gateway response error"));
                } else {
                    const QJsonObject obj = doc.object();
                    const QString cfg = obj.value(QStringLiteral("config")).toString();
                    if (cfg == QStringLiteral("timeout")) {
                        QThread::msleep(pollIntervalMs);
                        continue;
                    }
                    if (!cfg.isEmpty()) {
                        QMetaObject::invokeMethod(this, [this, cfg]() {
                            if (!m_importController)
                                return;

                            if (!m_importController->extractConfigFromData(cfg)) {
                                emit waitError(QStringLiteral("Invalid config received from gateway"));
                                return;
                            }

                            m_importController->importConfig();
                            emit configApplied();
                        }, Qt::QueuedConnection);
                        break;
                    }
                }
            } else {
                emit waitError(QStringLiteral("Network error"));
            }

            QThread::msleep(pollIntervalMs);
            //QThread::msleep(static_cast<unsigned long>(backoffMs));
            //backoffMs = qMin(backoffMs * 2, maxBackoffMs);
        }
    });
}

void TransferController::stopWaitForConfig()
{
    qDebug() << "TransferController::stopWaitForConfig: stop flag set";
    m_stopWaiting.storeRelease(1);
}
