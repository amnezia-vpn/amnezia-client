#include "apiController.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent>

#include "QRsa.h"

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

        constexpr char countryCode[] = "country_code";
        constexpr char serviceType[] = "service_type";

        constexpr char encryptPublicKey[] = "encrypt_public_key";
    }

    ErrorCode checkErrors(const QList<QSslError> &sslErrors, QNetworkReply *reply)
    {
        if (!sslErrors.empty()) {
            qDebug().noquote() << sslErrors;
            return ErrorCode::ApiConfigSslError;
        } else if (reply->error() == QNetworkReply::NoError) {
            return ErrorCode::NoError;
        } else if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError
                   || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
            return ErrorCode::ApiConfigTimeoutError;
        } else {
            QString err = reply->errorString();
            qDebug() << QString::fromUtf8(reply->readAll());
            qDebug() << reply->error();
            qDebug() << err;
            qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            return ErrorCode::ApiConfigDownloadError;
        }
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
        auto serverConfig = QJsonDocument::fromJson(config.toUtf8()).object();
        auto containers = serverConfig.value(config_key::containers).toArray();
        if (containers.isEmpty()) {
            return;
        }
        auto container = containers.at(0).toObject();
        QString containerName = ContainerProps::containerTypeToString(DockerContainer::Awg);
        auto containerConfig = container.value(containerName).toObject();
        auto protocolConfig = QJsonDocument::fromJson(containerConfig.value(config_key::last_config).toString().toUtf8()).object();
        containerConfig[config_key::junkPacketCount] = protocolConfig.value(config_key::junkPacketCount);
        containerConfig[config_key::junkPacketMinSize] = protocolConfig.value(config_key::junkPacketMinSize);
        containerConfig[config_key::junkPacketMaxSize] = protocolConfig.value(config_key::junkPacketMaxSize);
        containerConfig[config_key::initPacketJunkSize] = protocolConfig.value(config_key::initPacketJunkSize);
        containerConfig[config_key::responsePacketJunkSize] = protocolConfig.value(config_key::responsePacketJunkSize);
        containerConfig[config_key::initPacketMagicHeader] = protocolConfig.value(config_key::initPacketMagicHeader);
        containerConfig[config_key::responsePacketMagicHeader] = protocolConfig.value(config_key::responsePacketMagicHeader);
        containerConfig[config_key::underloadPacketMagicHeader] = protocolConfig.value(config_key::underloadPacketMagicHeader);
        containerConfig[config_key::transportPacketMagicHeader] = protocolConfig.value(config_key::transportPacketMagicHeader);
        container[containerName] = containerConfig;
        containers.replace(0, container);
        serverConfig[config_key::containers] = containers;
        config = QString(QJsonDocument(serverConfig).toJson());
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
                qDebug() << QJsonDocument::fromJson(configStr.toUtf8()).object();
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

ErrorCode ApiController::getServicesList(QByteArray &responseBody)
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    QNetworkAccessManager manager;
    QNetworkRequest request;
    request.setTransferTimeout(7000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setUrl(QString("http://localhost:52525/v1/services"));

    QScopedPointer<QNetworkReply> reply;
    reply.reset(manager.get(request));

    QEventLoop wait;
    QObject::connect(reply.get(), &QNetworkReply::finished, &wait, &QEventLoop::quit);

    QList<QSslError> sslErrors;
    connect(reply.get(), &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
    wait.exec();

    responseBody = reply->readAll();

    return checkErrors(sslErrors, reply.get());
}

ErrorCode ApiController::getConfigForService(const QString &installationUuid, const QString &countryCode, const QString &serviceType,
                                             const QString &protocol, QByteArray &responseBody)
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    QNetworkAccessManager manager;
    QNetworkRequest request;
    request.setTransferTimeout(7000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setUrl(QString("http://localhost:52525/v1/config"));

    ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

    QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
    apiPayload[configKey::countryCode] = countryCode;
    apiPayload[configKey::serviceType] = serviceType;
    apiPayload[configKey::uuid] = installationUuid;

    QByteArray localPublicKey;
    QByteArray localPrivateKey;

    try {
        QSimpleCrypto::QRsa rsa;
        EVP_PKEY *key = rsa.generateRsaKeys(2048, 3);
        localPublicKey = rsa.savePublicKeyToByteArray(key);
        localPrivateKey = rsa.savePrivateKeyToByteArray(key, "123456", EVP_aes_256_cbc());
        EVP_PKEY_free(key);
    } catch (const std::runtime_error &e) {
        qCritical() << "error when encrypting the request body" << e.what();
    } catch (...) {
        qCritical() << "error when encrypting the request body";
    }

    apiPayload[configKey::encryptPublicKey] = QString(localPublicKey.toBase64());

    QByteArray requestBody = QJsonDocument(apiPayload).toJson();

    QByteArray encryptedRequestBody;
    try {
        QSimpleCrypto::QRsa rsa;
        EVP_PKEY *publicKey = rsa.getPublicKeyFromFile("testkeys.pem");
        encryptedRequestBody = rsa.encrypt(requestBody, publicKey, RSA_PKCS1_PADDING);
        EVP_PKEY_free(publicKey);
    } catch (const std::runtime_error &e) {
        qCritical() << "error when encrypting the request body" << e.what();
    } catch (...) {
        qCritical() << "error when encrypting the request body";
    }

    QScopedPointer<QNetworkReply> reply;
    reply.reset(manager.post(request, encryptedRequestBody));

    QEventLoop wait;
    connect(reply.get(), &QNetworkReply::finished, &wait, &QEventLoop::quit);

    QList<QSslError> sslErrors;
    connect(reply.get(), &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
    wait.exec();

    auto encryptedResponseBody = reply->readAll();
    try {
        QSimpleCrypto::QRsa rsa;
        EVP_PKEY *privateKey = rsa.getPrivateKeyFromByteArray(localPrivateKey, "123456");
        responseBody = rsa.decrypt(encryptedResponseBody, privateKey, RSA_PKCS1_PADDING);
        EVP_PKEY_free(privateKey);
    } catch (const std::runtime_error &e) {
        qCritical() << "error when decrypting the request body" << e.what();
    } catch (...) {
        qCritical() << "error when decrypting the request body";
    }

    responseBody = reply->readAll();

    return checkErrors(sslErrors, reply.get());
}
