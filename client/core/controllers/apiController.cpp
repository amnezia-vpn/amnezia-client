#include "apiController.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent>

#include "amnezia_application.h"
#include "ui/controllers/connectionController.h"
#include "configurators/wireguard_configurator.h"

namespace
{
    namespace configKey
    {
        constexpr char cloak[] = "cloak";
        constexpr char awg[] = "awg";

        constexpr char apiEdnpoint[] = "api_endpoint";
        constexpr char accessToken[] = "api_key";
        constexpr char certificate[] = "certificate";
        constexpr char publicKey[] = "public_key";
        constexpr char protocol[] = "protocol";
        constexpr char uuid[] = "installation_uuid";
    }
}

ApiController::ApiController(ConnectionController *connectionController) :
    m_connectionController(connectionController),
    QObject(connectionController)
{
}

void ApiController::processApiConfig(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData,
                                     QString &config)
{
    if (protocol == configKey::cloak) {
        config.replace("<key>", "<key>\n");
        config.replace("$OPENVPN_PRIV_KEY", apiPayloadData.certRequest.privKey);
        return;
    } else if (protocol == configKey::awg) {
        config.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", apiPayloadData.wireGuardClientPrivKey);
    }
    return;
}

ApiController::ApiPayloadData ApiController::generateApiPayloadData(const QString &protocol)
{
    ApiController::ApiPayloadData apiPayload;
    if (protocol == configKey::cloak) {
        apiPayload.certRequest = OpenVpnConfigurator::createCertRequest();
    } else if (protocol == configKey::awg) {
        auto connData = WireguardConfigurator::genClientKeys();
        apiPayload.wireGuardClientPubKey = connData.clientPubKey;
        apiPayload.wireGuardClientPrivKey = connData.clientPrivKey;
    }
    return apiPayload;
}

QJsonObject ApiController::fillApiPayload(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData)
{
    QJsonObject obj;
    if (protocol == configKey::cloak) {
        obj[configKey::certificate] = apiPayloadData.certRequest.request;
    } else if (protocol == configKey::awg) {
        obj[configKey::publicKey] = apiPayloadData.wireGuardClientPubKey;
    }
    return obj;
}

void ApiController::updateServerConfigFromApi(const QString &installationUuid, const QJsonObject &serverConfig,
                                              const std::function<void (bool updateConfig, QJsonObject config)> &cb)
{
#ifdef Q_OS_IOS
    m_mobileUtils.requestInetAccess();
    QThread::msleep(10);
#endif

    auto containerConfig = serverConfig.value(config_key::containers).toArray();

    if (serverConfig.value(config_key::configVersion).toInt()) {
        QNetworkRequest request;
        request.setTransferTimeout(7000);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization",
                             "Api-Key " + serverConfig.value(configKey::accessToken).toString().toUtf8());
        QString endpoint = serverConfig.value(configKey::apiEdnpoint).toString();
        request.setUrl(endpoint);

        QString protocol = serverConfig.value(configKey::protocol).toString();

        ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

        QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
        apiPayload[configKey::uuid] = installationUuid;

        QByteArray requestBody = QJsonDocument(apiPayload).toJson();

        QNetworkReply *reply = amnApp->manager()->post(request, requestBody);

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, protocol, apiPayloadData, cb, config=serverConfig]() mutable {
            if (reply->error() == QNetworkReply::NoError) {
                QString contents = QString::fromUtf8(reply->readAll());
                QString data = QJsonDocument::fromJson(contents.toUtf8()).object().value(config_key::config).toString();

                data.replace("vpn://", "");
                QByteArray ba = QByteArray::fromBase64(data.toUtf8(),
                                                       QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

                if (ba.isEmpty()) {
                    qDebug() << "ApiController::updateServerConfigFromApi empty config";
                    emit m_connectionController->connectionErrorOccurred(tr("The selected protocol is not supported on the current platform"));

                    return; // ErrorCode::ApiConfigDownloadError;
                }

                QByteArray ba_uncompressed = qUncompress(ba);
                if (!ba_uncompressed.isEmpty()) {
                    ba = ba_uncompressed;
                }

                QString configStr = ba;
                processApiConfig(protocol, apiPayloadData, configStr);

                QJsonObject apiConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();

                config.insert(config_key::dns1, apiConfig.value(config_key::dns1));
                config.insert(config_key::dns2, apiConfig.value(config_key::dns2));
                config.insert(config_key::containers, apiConfig.value(config_key::containers));
                config.insert(config_key::hostName, apiConfig.value(config_key::hostName));

                auto defaultContainer = apiConfig.value(config_key::defaultContainer).toString();
                config.insert(config_key::defaultContainer, defaultContainer);

                cb(true, config);
            } else {
                if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
                    emit m_connectionController->connectionErrorOccurred("API timeout error");
                }
                else {
                    QString err = reply->errorString();
                    qDebug() << QString::fromUtf8(reply->readAll());
                    qDebug() << reply->error();
                    qDebug() << err;
                    qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                    emit m_connectionController->connectionErrorOccurred(reply->errorString());
                    // return ErrorCode::ApiConfigDownloadError;
                }
            }

            reply->deleteLater();
        });

        QObject::connect(reply, &QNetworkReply::errorOccurred, [this, reply](QNetworkReply::NetworkError error){
            qDebug() << reply->errorString() << error;
        });
        connect(reply, &QNetworkReply::sslErrors, [this, reply](const QList<QSslError> &errors){
            qDebug().noquote() << errors;
            emit m_connectionController->connectionErrorOccurred("Ssl errors");
        });
    }
}
