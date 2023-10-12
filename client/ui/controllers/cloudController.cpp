#include "cloudController.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

#include "configurators/openvpn_configurator.h"

CloudController::CloudController(const QSharedPointer<ServersModel> &serversModel,
                                 const QSharedPointer<ContainersModel> &containersModel, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel)
{
}

QString CloudController::genPublicKey(ServiceTypeId serviceTypeId)
{
    switch (serviceTypeId)
    {
        case ServiceTypeId::AmneziaFreeRuWG: return ".";
        case ServiceTypeId::AmneziaFreeRuCloak: return ".";
        case ServiceTypeId::AmneziaFreeRuAWG: return ".";
        case ServiceTypeId::AmneziaFreeRuReverseWG: return ".";
        case ServiceTypeId::AmneziaFreeRuReverseCloak: return ".";
        case ServiceTypeId::AmneziaFreeRuReverseAWG: return ".";
    }
}

QString CloudController::genCertificateRequest(ServiceTypeId serviceTypeId)
{
    switch (serviceTypeId)
    {
        case ServiceTypeId::AmneziaFreeRuWG: return "";
        case ServiceTypeId::AmneziaFreeRuCloak: {
            auto data = OpenVpnConfigurator::createCertRequest();
            return data.request;
        }
        case ServiceTypeId::AmneziaFreeRuAWG: return "";
        case ServiceTypeId::AmneziaFreeRuReverseWG: return "";
        case ServiceTypeId::AmneziaFreeRuReverseCloak: return "";
        case ServiceTypeId::AmneziaFreeRuReverseAWG: return "";
    }
}

bool CloudController::updateServerConfigFromCloud()
{
    auto serverConfig = m_serversModel->getDefaultServerConfig();

    auto containerConfig = serverConfig.value(config_key::containers).toArray();

    if (serverConfig.value(config_key::configVersion).toInt() && containerConfig.isEmpty()) {
        QNetworkAccessManager manager;

        QNetworkRequest request;
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QString endpoint = serverConfig.value(config_key::apiEdnpoint).toString();
        request.setUrl(endpoint.replace("https", "http")); //

        ServiceTypeId serviceTypeId = static_cast<ServiceTypeId>(serverConfig.value(config_key::serviceTypeId).toInt());

        QJsonObject obj;
        obj[config_key::serviceTypeId] = serviceTypeId;
        obj[config_key::accessToken] =  serverConfig.value(config_key::accessToken);

        obj[config_key::publicKey] = genPublicKey(serviceTypeId);
        obj[config_key::certificate] = genCertificateRequest(serviceTypeId);

        QByteArray requestBody = QJsonDocument(obj).toJson();

        QScopedPointer<QNetworkReply> reply;
        reply.reset(manager.post(request, requestBody));

        QEventLoop wait;
        QObject::connect(reply.get(), &QNetworkReply::finished, &wait, &QEventLoop::quit);
        wait.exec();

        if(reply->error() == QNetworkReply::NoError){
            QString contents = QString::fromUtf8(reply->readAll());
            auto data = QJsonDocument::fromJson(contents.toUtf8()).object().value(config_key::config).toString();

            data.replace("vpn://", "");
            QByteArray ba = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

            QByteArray ba_uncompressed = qUncompress(ba);
            if (!ba_uncompressed.isEmpty()) {
                ba = ba_uncompressed;
            }

            QJsonObject cloudConfig = QJsonDocument::fromJson(ba).object();
            serverConfig.insert("cloudConfig", cloudConfig);
            serverConfig.insert(config_key::dns1, cloudConfig.value(config_key::dns1));
            serverConfig.insert(config_key::dns2, cloudConfig.value(config_key::dns2));
            serverConfig.insert(config_key::containers, cloudConfig.value(config_key::containers));
            serverConfig.insert(config_key::hostName, cloudConfig.value(config_key::hostName));
            serverConfig.insert(config_key::defaultContainer, cloudConfig.value(config_key::defaultContainer));
            m_serversModel->editServer(serverConfig);
            emit serverConfigUpdated();
        } else{
            QString err = reply->errorString();
            qDebug() << reply->error();
            qDebug() << err;
            qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            emit errorOccurred(tr("Error when retrieving configuration from cloud server"));
            return false;
        }

    }

    return true;
}
