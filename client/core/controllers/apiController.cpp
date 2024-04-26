#include "apiController.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent>

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

ApiController::ApiController(QObject *parent) : QObject(parent)
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

ErrorCode ApiController::updateServerConfigFromApi(const QString &installationUuid, QJsonObject &serverConfig)
{
    QFutureWatcher<ErrorCode> watcher;

    QFuture<ErrorCode> future = QtConcurrent::run([this, &serverConfig, &installationUuid]() {
        auto containerConfig = serverConfig.value(config_key::containers).toArray();

        if (serverConfig.value(config_key::configVersion).toInt()) {
            QNetworkAccessManager manager;

            QNetworkRequest request;
            request.setTransferTimeout(7000);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            request.setRawHeader("Authorization",
                                 "Api-Key " + serverConfig.value(configKey::accessToken).toString().toUtf8());
            QString endpoint = serverConfig.value(configKey::apiEdnpoint).toString();
            request.setUrl(endpoint);

            QString protocol = serverConfig.value(configKey::protocol).toString();

            auto apiPayloadData = generateApiPayloadData(protocol);

            auto apiPayload = fillApiPayload(protocol, apiPayloadData);
            apiPayload[configKey::uuid] = installationUuid;

            QByteArray requestBody = QJsonDocument(apiPayload).toJson();

            QScopedPointer<QNetworkReply> reply;
            reply.reset(manager.post(request, requestBody));

            QEventLoop wait;
            QObject::connect(reply.get(), &QNetworkReply::finished, &wait, &QEventLoop::quit);
            wait.exec();

            if (reply->error() == QNetworkReply::NoError) {
                QString contents = QString::fromUtf8(reply->readAll());
                auto data = QJsonDocument::fromJson(contents.toUtf8()).object().value(config_key::config).toString();

                data.replace("vpn://", "");
                QByteArray ba = QByteArray::fromBase64(data.toUtf8(),
                                                       QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

                if (ba.isEmpty()) {
                    return ErrorCode::ApiConfigDownloadError;
                }

                QByteArray ba_uncompressed = qUncompress(ba);
                if (!ba_uncompressed.isEmpty()) {
                    ba = ba_uncompressed;
                }

                QString configStr = ba;
                processApiConfig(protocol, apiPayloadData, configStr);

                QJsonObject apiConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();

                serverConfig.insert(config_key::dns1, apiConfig.value(config_key::dns1));
                serverConfig.insert(config_key::dns2, apiConfig.value(config_key::dns2));
                serverConfig.insert(config_key::containers, apiConfig.value(config_key::containers));
                serverConfig.insert(config_key::hostName, apiConfig.value(config_key::hostName));

                auto defaultContainer = apiConfig.value(config_key::defaultContainer).toString();
                serverConfig.insert(config_key::defaultContainer, defaultContainer);
            } else {
                QString err = reply->errorString();
                qDebug() << QString::fromUtf8(reply->readAll());
                qDebug() << reply->error();
                qDebug() << err;
                qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                return ErrorCode::ApiConfigDownloadError;
            }
        }
        return ErrorCode::NoError;
    });

    QEventLoop wait;
    connect(&watcher, &QFutureWatcher<ErrorCode>::finished, &wait, &QEventLoop::quit);
    watcher.setFuture(future);
    wait.exec();

    return watcher.result();
}
