#include "apiController.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent>

#include "QBlockCipher.h"
#include "QRsa.h"

#include "amnezia_application.h"
#include "configurators/wireguard_configurator.h"
#include "core/enums/apiEnums.h"
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

        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serviceType[] = "service_type";

        constexpr char aesKey[] = "aes_key";
        constexpr char aesIv[] = "aes_iv";
        constexpr char aesSalt[] = "aes_salt";

        constexpr char apiPayload[] = "api_payload";
        constexpr char keyPayload[] = "key_payload";
    }

    const QStringList proxyStorageUrl = { "" };

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

ApiController::ApiController(const QString &gatewayEndpoint, bool isDevEnvironment, QObject *parent)
    : QObject(parent), m_gatewayEndpoint(gatewayEndpoint), m_isDevEnvironment(isDevEnvironment)
{
}

void ApiController::fillServerConfig(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData,
                                     const QByteArray &apiResponseBody, QJsonObject &serverConfig)
{
    QString data = QJsonDocument::fromJson(apiResponseBody).object().value(config_key::config).toString();

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
    if (protocol == configKey::cloak) {
        configStr.replace("<key>", "<key>\n");
        configStr.replace("$OPENVPN_PRIV_KEY", apiPayloadData.certRequest.privKey);
    } else if (protocol == configKey::awg) {
        configStr.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", apiPayloadData.wireGuardClientPrivKey);
        auto serverConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
        auto containers = serverConfig.value(config_key::containers).toArray();
        if (containers.isEmpty()) {
            return; // todo process error
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
        configStr = QString(QJsonDocument(serverConfig).toJson());
    }

    QJsonObject apiConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
    serverConfig[config_key::dns1] = apiConfig.value(config_key::dns1);
    serverConfig[config_key::dns2] = apiConfig.value(config_key::dns2);
    serverConfig[config_key::containers] = apiConfig.value(config_key::containers);
    serverConfig[config_key::hostName] = apiConfig.value(config_key::hostName);

    if (apiConfig.value(config_key::configVersion).toInt() == ApiConfigSources::AmneziaGateway) {
        serverConfig[config_key::configVersion] = apiConfig.value(config_key::configVersion);
        serverConfig[config_key::description] = apiConfig.value(config_key::description);
        serverConfig[config_key::name] = apiConfig.value(config_key::name);
    }

    auto defaultContainer = apiConfig.value(config_key::defaultContainer).toString();
    serverConfig[config_key::defaultContainer] = defaultContainer;

    return;
}

QStringList ApiController::getProxyUrls()
{
    QNetworkRequest request;
    request.setTransferTimeout(7000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QEventLoop wait;
    QList<QSslError> sslErrors;
    QNetworkReply *reply;

    for (const auto &proxyStorageUrl : proxyStorageUrl) {
        request.setUrl(proxyStorageUrl);
        reply = amnApp->manager()->get(request);

        connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
        connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
        wait.exec();

        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            break;
        }
        reply->deleteLater();
    }

    auto encryptedResponseBody = reply->readAll();
    reply->deleteLater();

    EVP_PKEY *privateKey = nullptr;
    QByteArray responseBody;
    try {
        QByteArray key = PROD_PROXY_STORAGE_KEY;
        QSimpleCrypto::QRsa rsa;
        privateKey = rsa.getPrivateKeyFromByteArray(key, "");
        responseBody = rsa.decrypt(encryptedResponseBody, privateKey, RSA_PKCS1_PADDING);
    } catch (...) {
        qCritical() << "error loading private key from environment variables or decrypting payload";
        return {};
    }

    auto endpointsArray = QJsonDocument::fromJson(responseBody).array();

    QStringList endpoints;
    for (const auto &endpoint : endpointsArray) {
        endpoints.push_back(endpoint.toString());
    }
    return endpoints;
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

        QNetworkReply *reply = amnApp->manager()->post(request, requestBody);

        QObject::connect(reply, &QNetworkReply::finished, [this, reply, protocol, apiPayloadData, serverIndex, serverConfig]() mutable {
            if (reply->error() == QNetworkReply::NoError) {
                auto apiResponseBody = reply->readAll();
                fillServerConfig(protocol, apiPayloadData, apiResponseBody, serverConfig);
                emit finished(serverConfig, serverIndex);
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

    QNetworkRequest request;
    request.setTransferTimeout(7000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setUrl(QString("%1v1/services").arg(m_gatewayEndpoint));

    QNetworkReply *reply;
    reply = amnApp->manager()->get(request);

    QEventLoop wait;
    QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);

    QList<QSslError> sslErrors;
    connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
    wait.exec();

    if (reply->error() == QNetworkReply::NetworkError::TimeoutError || reply->error() == QNetworkReply::NetworkError::OperationCanceledError) {
        m_proxyUrls = getProxyUrls();
        for (const QString &proxyUrl : m_proxyUrls) {
            request.setUrl(QString("%1v1/services").arg(proxyUrl));
            reply = amnApp->manager()->get(request);

            QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
            connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
            wait.exec();
            if (reply->error() != QNetworkReply::NetworkError::TimeoutError
                && reply->error() != QNetworkReply::NetworkError::OperationCanceledError) {
                break;
            }
            reply->deleteLater();
        }
    }

    responseBody = reply->readAll();
    auto errorCode = checkErrors(sslErrors, reply);
    reply->deleteLater();
    return errorCode;
}

ErrorCode ApiController::getConfigForService(const QString &installationUuid, const QString &userCountryCode, const QString &serviceType,
                                             const QString &protocol, const QString &serverCountryCode, QJsonObject &serverConfig)
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    QNetworkAccessManager manager;
    QNetworkRequest request;
    request.setTransferTimeout(7000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setUrl(QString("%1v1/config").arg(m_gatewayEndpoint));

    ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

    QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
    apiPayload[configKey::userCountryCode] = userCountryCode;
    if (!serverCountryCode.isEmpty()) {
        apiPayload[configKey::serverCountryCode] = serverCountryCode;
    }
    apiPayload[configKey::serviceType] = serviceType;
    apiPayload[configKey::uuid] = installationUuid;

    QSimpleCrypto::QBlockCipher blockCipher;
    QByteArray key = blockCipher.generatePrivateSalt(32);
    QByteArray iv = blockCipher.generatePrivateSalt(32);
    QByteArray salt = blockCipher.generatePrivateSalt(8);

    QJsonObject keyPayload;
    keyPayload[configKey::aesKey] = QString(key.toBase64());
    keyPayload[configKey::aesIv] = QString(iv.toBase64());
    keyPayload[configKey::aesSalt] = QString(salt.toBase64());

    QByteArray encryptedKeyPayload;
    QByteArray encryptedApiPayload;
    try {
        QSimpleCrypto::QRsa rsa;

        EVP_PKEY *publicKey = nullptr;
        try {
            QByteArray key = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
            QSimpleCrypto::QRsa rsa;
            publicKey = rsa.getPublicKeyFromByteArray(key);
        } catch (...) {
            qCritical() << "error loading public key from environment variables";
            return ErrorCode::ApiMissingAgwPublicKey;
        }

        encryptedKeyPayload = rsa.encrypt(QJsonDocument(keyPayload).toJson(), publicKey, RSA_PKCS1_PADDING);
        EVP_PKEY_free(publicKey);

        encryptedApiPayload = blockCipher.encryptAesBlockCipher(QJsonDocument(apiPayload).toJson(), key, iv, "", salt);
    } catch (...) { // todo change error handling in QSimpleCrypto?
        qCritical() << "error when encrypting the request body";
    }

    QJsonObject requestBody;
    requestBody[configKey::keyPayload] = QString(encryptedKeyPayload.toBase64());
    requestBody[configKey::apiPayload] = QString(encryptedApiPayload.toBase64());

    QNetworkReply *reply = manager.post(request, QJsonDocument(requestBody).toJson());

    QEventLoop wait;
    connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);

    QList<QSslError> sslErrors;
    connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
    wait.exec();

    if (reply->error() == QNetworkReply::NetworkError::TimeoutError || reply->error() == QNetworkReply::NetworkError::OperationCanceledError) {
        if (m_proxyUrls.isEmpty()) {
            m_proxyUrls = getProxyUrls();
        }
        for (const QString &proxyUrl : m_proxyUrls) {
            request.setUrl(QString("%1v1/config").arg(proxyUrl));
            reply = manager.post(request, QJsonDocument(requestBody).toJson());

            QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
            connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
            wait.exec();
            if (reply->error() != QNetworkReply::NetworkError::TimeoutError
                && reply->error() != QNetworkReply::NetworkError::OperationCanceledError) {
                break;
            }
            reply->deleteLater();
        }
    }

    auto errorCode = checkErrors(sslErrors, reply);
    if (errorCode) {
        return errorCode;
    }

    auto encryptedResponseBody = reply->readAll();
    reply->deleteLater();
    try {
        auto responseBody = blockCipher.decryptAesBlockCipher(encryptedResponseBody, key, iv, "", salt);
        fillServerConfig(protocol, apiPayloadData, responseBody, serverConfig);
    } catch (...) { // todo change error handling in QSimpleCrypto?
        qCritical() << "error when decrypting the request body";
    }

    return errorCode;
}
