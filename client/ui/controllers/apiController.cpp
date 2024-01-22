#include "apiController.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "configurators/openvpn_configurator.h"
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
    }
}

ApiController::ApiController(const QSharedPointer<ServersModel> &serversModel,
                             const QSharedPointer<ContainersModel> &containersModel, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel)
{
}

void ApiController::processCloudConfig(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData, QString &config)
{
    if (protocol == configKey::cloak) {
        config.replace("<key>", "<key>\n");
        config.replace("$OPENVPN_PRIV_KEY", apiPayloadData.certRequest.privKey);
        return;
    } else if (protocol == configKey::awg) {
        config.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", apiPayloadData.wireGUardClientPubKey);
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
        apiPayload.wireGUardClientPubKey = connData.clientPubKey;
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
        obj[configKey::publicKey] = apiPayloadData.wireGUardClientPubKey;
    }
    return obj;
}

bool ApiController::updateServerConfigFromApi()
{
    auto serverConfig = m_serversModel->getDefaultServerConfig();

    auto containerConfig = serverConfig.value(config_key::containers).toArray();

    if (serverConfig.value(config_key::configVersion).toInt() && containerConfig.isEmpty()) {
        QNetworkAccessManager manager;

        QNetworkRequest request;
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization",
                             "Api-Key " + serverConfig.value(configKey::accessToken).toString().toUtf8());
        QString endpoint = serverConfig.value(configKey::apiEdnpoint).toString();
        request.setUrl(endpoint.replace("https", "http")); // todo remove

        QString protocol = serverConfig.value(configKey::protocol).toString();

        auto apiPayloadData = generateApiPayloadData(protocol);

        QByteArray requestBody = QJsonDocument(fillApiPayload(protocol, apiPayloadData)).toJson();

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

            QByteArray ba_uncompressed = qUncompress(ba);
            if (!ba_uncompressed.isEmpty()) {
                ba = ba_uncompressed;
            }

            QString configStr = ba;
            processCloudConfig(protocol, apiPayloadData, configStr);

            QJsonObject cloudConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();

            serverConfig.insert("cloudConfig", cloudConfig);
            serverConfig.insert(config_key::dns1, cloudConfig.value(config_key::dns1));
            serverConfig.insert(config_key::dns2, cloudConfig.value(config_key::dns2));
            serverConfig.insert(config_key::containers, cloudConfig.value(config_key::containers));
            serverConfig.insert(config_key::hostName, cloudConfig.value(config_key::hostName));

            auto defaultContainer = cloudConfig.value(config_key::defaultContainer).toString();
            serverConfig.insert(config_key::defaultContainer, defaultContainer);
            m_serversModel->editServer(serverConfig);
            emit m_serversModel->defaultContainerChanged(ContainerProps::containerFromString(defaultContainer));
        } else {
            QString err = reply->errorString();
            qDebug() << QString::fromUtf8(reply->readAll()); //todo remove debug output
            qDebug() << reply->error();
            qDebug() << err;
            qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            emit errorOccurred(tr("Error when retrieving configuration from cloud server"));
            return false;
        }
    }

    return true;
}
