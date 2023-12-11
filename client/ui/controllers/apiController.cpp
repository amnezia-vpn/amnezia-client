#include "apiController.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "configurators/openvpn_configurator.h"

namespace
{
    namespace configKey
    {
        constexpr char cloak[] = "cloak";

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

QString ApiController::genPublicKey(const QString &protocol)
{
    if (protocol == configKey::cloak) {
        return ".";
    }
    return QString();
}

QString ApiController::genCertificateRequest(const QString &protocol)
{
    if (protocol == configKey::cloak) {
        m_certRequest = OpenVpnConfigurator::createCertRequest();
        return m_certRequest.request;
    }
    return QString();
}

void ApiController::processCloudConfig(const QString &protocol, QString &config)
{
    if (protocol == configKey::cloak) {
        config.replace("<key>", "<key>\n");
        config.replace("$OPENVPN_PRIV_KEY", m_certRequest.privKey);
        return;
    }
    return;
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

        QJsonObject obj;

        obj[configKey::publicKey] = genPublicKey(protocol);
        obj[configKey::certificate] = genCertificateRequest(protocol);

        QByteArray requestBody = QJsonDocument(obj).toJson();
        qDebug() << requestBody;

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
            processCloudConfig(protocol, configStr);

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
