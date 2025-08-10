#include "gatewayController.h"

#include <algorithm>
#include <random>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>

#include "QBlockCipher.h"
#include "QRsa.h"

#include "amnezia_application.h"
#include "core/api/apiUtils.h"
#include "core/networkUtilities.h"
#include "utilities.h"

#ifdef AMNEZIA_DESKTOP
    #include "core/ipcclient.h"
#endif

namespace
{
    namespace configKey
    {
        constexpr char aesKey[] = "aes_key";
        constexpr char aesIv[] = "aes_iv";
        constexpr char aesSalt[] = "aes_salt";

        constexpr char apiPayload[] = "api_payload";
        constexpr char keyPayload[] = "key_payload";
    }

    constexpr QLatin1String errorResponsePattern1("No active configuration found for");
    constexpr QLatin1String errorResponsePattern2("No non-revoked public key found for");
    constexpr QLatin1String errorResponsePattern3("Account not found.");

    constexpr QLatin1String updateRequestResponsePattern("client version update is required");
}

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, QObject *parent)
    : QObject(parent),
      m_gatewayEndpoint(gatewayEndpoint),
      m_isDevEnvironment(isDevEnvironment),
      m_requestTimeoutMsecs(requestTimeoutMsecs),
      m_isStrictKillSwitchEnabled(isStrictKillSwitchEnabled)
{
}

ErrorCode GatewayController::get(const QString &endpoint, QByteArray &responseBody)
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    QNetworkRequest request;
    request.setTransferTimeout(m_requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setUrl(QString(endpoint).arg(m_gatewayEndpoint));

    // bypass killSwitch exceptions for API-gateway
#ifdef AMNEZIA_DESKTOP
    if (m_isStrictKillSwitchEnabled) {
        QString host = QUrl(request.url()).host();
        QString ip = NetworkUtilities::getIPAddress(host);
        if (!ip.isEmpty()) {
            IpcClient::Interface()->addKillSwitchAllowedRange(QStringList { ip });
        }
    }
#endif

    QNetworkReply *reply;
    reply = amnApp->networkManager()->get(request);

    QEventLoop wait;
    QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);

    QList<QSslError> sslErrors;
    connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
    wait.exec();

    responseBody = reply->readAll();

    if (sslErrors.isEmpty() && shouldBypassProxy(reply, responseBody, false)) {
        auto requestFunction = [&request, &responseBody](const QString &url) {
            request.setUrl(url);
            return amnApp->networkManager()->get(request);
        };

        auto replyProcessingFunction = [&responseBody, &reply, &sslErrors, this](QNetworkReply *nestedReply,
                                                                                 const QList<QSslError> &nestedSslErrors) {
            responseBody = nestedReply->readAll();
            if (!sslErrors.isEmpty() || !shouldBypassProxy(nestedReply, responseBody, false)) {
                sslErrors = nestedSslErrors;
                reply = nestedReply;
                return true;
            }
            return false;
        };

        bypassProxy(endpoint, reply, requestFunction, replyProcessingFunction);
    }

    auto errorCode = apiUtils::checkNetworkReplyErrors(sslErrors, reply);
    reply->deleteLater();

    return errorCode;
}

ErrorCode GatewayController::post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    QNetworkRequest request;
    request.setTransferTimeout(m_requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setUrl(endpoint.arg(m_gatewayEndpoint));

    // bypass killSwitch exceptions for API-gateway
#ifdef AMNEZIA_DESKTOP
    if (m_isStrictKillSwitchEnabled) {
        QString host = QUrl(request.url()).host();
        QString ip = NetworkUtilities::getIPAddress(host);
        if (!ip.isEmpty()) {
            IpcClient::Interface()->addKillSwitchAllowedRange(QStringList { ip });
        }
    }
#endif

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
            QByteArray rsaKey = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
            QSimpleCrypto::QRsa rsa;
            publicKey = rsa.getPublicKeyFromByteArray(rsaKey);
        } catch (...) {
            Utils::logException();
            qCritical() << "error loading public key from environment variables";
            return ErrorCode::ApiMissingAgwPublicKey;
        }

        encryptedKeyPayload = rsa.encrypt(QJsonDocument(keyPayload).toJson(), publicKey, RSA_PKCS1_PADDING);
        EVP_PKEY_free(publicKey);

        encryptedApiPayload = blockCipher.encryptAesBlockCipher(QJsonDocument(apiPayload).toJson(), key, iv, "", salt);
    } catch (...) { // todo change error handling in QSimpleCrypto?
        Utils::logException();
        qCritical() << "error when encrypting the request body";
        return ErrorCode::ApiConfigDecryptionError;
    }

    QJsonObject requestBody;
    requestBody[configKey::keyPayload] = QString(encryptedKeyPayload.toBase64());
    requestBody[configKey::apiPayload] = QString(encryptedApiPayload.toBase64());

    QNetworkReply *reply = amnApp->networkManager()->post(request, QJsonDocument(requestBody).toJson());

    QEventLoop wait;
    connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);

    QList<QSslError> sslErrors;
    connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
    wait.exec();

    QByteArray encryptedResponseBody = reply->readAll();

    if (sslErrors.isEmpty() && shouldBypassProxy(reply, encryptedResponseBody, true, key, iv, salt)) {
        auto requestFunction = [&request, &encryptedResponseBody, &requestBody](const QString &url) {
            request.setUrl(url);
            return amnApp->networkManager()->post(request, QJsonDocument(requestBody).toJson());
        };

        auto replyProcessingFunction = [&encryptedResponseBody, &reply, &sslErrors, &key, &iv, &salt,
                                        this](QNetworkReply *nestedReply, const QList<QSslError> &nestedSslErrors) {
            encryptedResponseBody = nestedReply->readAll();
            reply = nestedReply;
            if (!sslErrors.isEmpty() || shouldBypassProxy(nestedReply, encryptedResponseBody, true, key, iv, salt)) {
                sslErrors = nestedSslErrors;
                return false;
            }
            return true;
        };

        bypassProxy(endpoint, reply, requestFunction, replyProcessingFunction);
    }

    auto errorCode = apiUtils::checkNetworkReplyErrors(sslErrors, reply);
    reply->deleteLater();
    if (errorCode) {
        return errorCode;
    }

    try {
        responseBody = blockCipher.decryptAesBlockCipher(encryptedResponseBody, key, iv, "", salt);
        return ErrorCode::NoError;
    } catch (...) { // todo change error handling in QSimpleCrypto?
        Utils::logException();
        qCritical() << "error when decrypting the request body";
        return ErrorCode::ApiConfigDecryptionError;
    }
}

QStringList GatewayController::getProxyUrls()
{
    QNetworkRequest request;
    request.setTransferTimeout(m_requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QEventLoop wait;
    QList<QSslError> sslErrors;
    QNetworkReply *reply;

    QStringList proxyStorageUrls;
    if (m_isDevEnvironment) {
        proxyStorageUrls = QString(DEV_S3_ENDPOINT).split(", ");
    } else {
        proxyStorageUrls = QString(PROD_S3_ENDPOINT).split(", ");
    }

    QByteArray key = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;

    for (const auto &proxyStorageUrl : proxyStorageUrls) {
        request.setUrl(proxyStorageUrl);
        reply = amnApp->networkManager()->get(request);

        connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
        connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
        wait.exec();

        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            auto encryptedResponseBody = reply->readAll();
            reply->deleteLater();

            EVP_PKEY *privateKey = nullptr;
            QByteArray responseBody;
            try {
                if (!m_isDevEnvironment) {
                    QCryptographicHash hash(QCryptographicHash::Sha512);
                    hash.addData(key);
                    QByteArray hashResult = hash.result().toHex();

                    QByteArray key = QByteArray::fromHex(hashResult.left(64));
                    QByteArray iv = QByteArray::fromHex(hashResult.mid(64, 32));

                    QByteArray ba = QByteArray::fromBase64(encryptedResponseBody);

                    QSimpleCrypto::QBlockCipher blockCipher;
                    responseBody = blockCipher.decryptAesBlockCipher(ba, key, iv);
                } else {
                    responseBody = encryptedResponseBody;
                }
            } catch (...) {
                Utils::logException();
                qCritical() << "error loading private key from environment variables or decrypting payload" << encryptedResponseBody;
                continue;
            }

            auto endpointsArray = QJsonDocument::fromJson(responseBody).array();

            QStringList endpoints;
            for (const auto &endpoint : endpointsArray) {
                endpoints.push_back(endpoint.toString());
            }
            return endpoints;
        } else {
            apiUtils::checkNetworkReplyErrors(sslErrors, reply);
            qDebug() << "go to the next storage endpoint";

            reply->deleteLater();
        }
    }
    return {};
}

bool GatewayController::shouldBypassProxy(QNetworkReply *reply, const QByteArray &responseBody, bool checkEncryption, const QByteArray &key,
                                          const QByteArray &iv, const QByteArray &salt)
{
    if (reply->error() == QNetworkReply::NetworkError::OperationCanceledError || reply->error() == QNetworkReply::NetworkError::TimeoutError) {
        qDebug() << "timeout occurred";
        qDebug() << reply->error();
        return true;
    } else if (responseBody.contains("html")) {
        qDebug() << "the response contains an html tag";
        return true;
    } else if (reply->error() == QNetworkReply::NetworkError::ContentNotFoundError) {
        if (responseBody.contains(errorResponsePattern1) || responseBody.contains(errorResponsePattern2)
            || responseBody.contains(errorResponsePattern3)) {
            return false;
        } else {
            qDebug() << reply->error();
            return true;
        }
    } else if (reply->error() == QNetworkReply::NetworkError::OperationNotImplementedError) {
        if (responseBody.contains(updateRequestResponsePattern)) {
            return false;
        } else {
            qDebug() << reply->error();
            return true;
        }
    } else if (reply->error() != QNetworkReply::NetworkError::NoError) {
        qDebug() << reply->error();
        return true;
    } else if (checkEncryption) {
        try {
            QSimpleCrypto::QBlockCipher blockCipher;
            static_cast<void>(blockCipher.decryptAesBlockCipher(responseBody, key, iv, "", salt));
        } catch (...) {
            qDebug() << "failed to decrypt the data";
            return true;
        }
    }
    return false;
}

void GatewayController::bypassProxy(const QString &endpoint, QNetworkReply *reply,
                                    std::function<QNetworkReply *(const QString &url)> requestFunction,
                                    std::function<bool(QNetworkReply *reply, const QList<QSslError> &sslErrors)> replyProcessingFunction)
{
    QStringList proxyUrls = getProxyUrls();
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::shuffle(proxyUrls.begin(), proxyUrls.end(), generator);

    QEventLoop wait;
    QList<QSslError> sslErrors;
    QByteArray responseBody;

    for (const QString &proxyUrl : proxyUrls) {
        qDebug() << "go to the next proxy endpoint";
        reply->deleteLater(); // delete the previous reply
        reply = requestFunction(endpoint.arg(proxyUrl));

        QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
        connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
        wait.exec();

        if (replyProcessingFunction(reply, sslErrors)) {
            break;
        }
    }
}
