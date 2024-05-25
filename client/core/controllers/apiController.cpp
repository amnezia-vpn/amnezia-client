#include "apiController.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent>

#include "amnezia_application.h"
#include "configurators/wireguard_configurator.h"
#include "version.h"

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
        constexpr char osVersion[] = "os_version";
        constexpr char appVersion[] = "app_version";
    }
}

ApiController::ApiController(QObject *parent) : QObject(parent)
{
}

void ApiController::processApiConfig(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData, QString &config)
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

    obj[configKey::osVersion] = QSysInfo::productType();
    obj[configKey::appVersion] = QString(APP_VERSION);

    return obj;
}

void ApiController::updateServerConfigFromApi(const QString &installationUuid, const int serverIndex, QJsonObject serverConfig)
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    auto containerConfig = serverConfig.value(config_key::containers).toArray();

    if (serverConfig.value(config_key::configVersion).toInt()) {
        QNetworkRequest request;
        request.setTransferTimeout(7000);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Api-Key " + serverConfig.value(configKey::accessToken).toString().toUtf8());
        QString endpoint = serverConfig.value(configKey::apiEdnpoint).toString();
        request.setUrl(endpoint);

        QString protocol = serverConfig.value(configKey::protocol).toString();

        ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

        QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
        apiPayload[configKey::uuid] = installationUuid;

        QByteArray requestBody = QJsonDocument(apiPayload).toJson();

        QNetworkReply *reply = amnApp->manager()->post(request, requestBody); // ??

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, protocol, apiPayloadData, serverIndex, serverConfig]() mutable {
            if (reply->error() == QNetworkReply::NoError) {
                QString contents = QString::fromUtf8(reply->readAll());
                QString data = QJsonDocument::fromJson(contents.toUtf8()).object().value(config_key::config).toString();

                data.replace("vpn://", "");
                QByteArray ba = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

                if (ba.isEmpty()) {
                    emit errorOccurred(ErrorCode::ApiConfigEmptyError);
                    return;
                }

                QByteArray ba_uncompressed = qUncompress(ba);
                if (!ba_uncompressed.isEmpty()) {
                    ba = ba_uncompressed;
                }

                QString configStr = ba;
                processApiConfig(protocol, apiPayloadData, configStr);

                QJsonObject apiConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
                serverConfig[config_key::dns1] = apiConfig.value(config_key::dns1);
                serverConfig[config_key::dns2] = apiConfig.value(config_key::dns2);
                serverConfig[config_key::containers] = apiConfig.value(config_key::containers);
                serverConfig[config_key::hostName] = apiConfig.value(config_key::hostName);

                auto defaultContainer = apiConfig.value(config_key::defaultContainer).toString();
                serverConfig[config_key::defaultContainer] = defaultContainer;

                emit configUpdated(true, serverConfig, serverIndex);
            } else {
                if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError
                    || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
                    emit errorOccurred(ErrorCode::ApiConfigTimeoutError);
                } else {
                    QString err = reply->errorString();
                    qDebug() << QString::fromUtf8(reply->readAll());
                    qDebug() << reply->error();
                    qDebug() << err;
                    qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                    emit errorOccurred(ErrorCode::ApiConfigDownloadError);
                }
            }

            reply->deleteLater();
        });

        QObject::connect(reply, &QNetworkReply::errorOccurred,
                         [this, reply](QNetworkReply::NetworkError error) { qDebug() << reply->errorString() << error; });
        connect(reply, &QNetworkReply::sslErrors, [this, reply](const QList<QSslError> &errors) {
            qDebug().noquote() << errors;
            emit errorOccurred(ErrorCode::ApiConfigSslError);
        });
    }
}
