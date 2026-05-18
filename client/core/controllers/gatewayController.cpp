#include "gatewayController.h"

#include <algorithm>
#include <functional>
#include <random>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPromise>
#include <QTimer>
#include <QUrl>

#include "QBlockCipher.h"
#include "QRsa.h"

#include "amneziaApplication.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/utilities.h"

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
#endif

namespace
{
    void execNetworkWaitLoop(QEventLoop &wait)
    {
#ifdef Q_OS_IOS
        // QEventLoop::ExcludeUserInputEvents is not supported on iOS (Qt warns; can break nested UI).
        wait.exec();
#else
        wait.exec(QEventLoop::ExcludeUserInputEvents);
#endif
    }

    constexpr QLatin1String errorResponsePattern1("No active configuration found for");
    constexpr QLatin1String errorResponsePattern2("No non-revoked public key found for");
    constexpr QLatin1String errorResponsePattern3("Account not found.");

    constexpr QLatin1String updateRequestResponsePattern("client version update is required");

    constexpr int httpStatusCodeNotFound = 404;
    constexpr int httpStatusCodeConflict = 409;
    constexpr int httpStatusCodeNotImplemented = 501;
    constexpr int httpStatusCodePaymentRequired = 402;
    constexpr int httpStatusCodeUnprocessableEntity = 422;

    constexpr QLatin1String unprocessableSubscriptionMessage("Failed to retrieve subscription information. Is it activated?");

    constexpr int proxyStorageRequestTimeoutMsecs = 3000;

    /** Gateway paths use "%1v1/..." — %1 must end with '/' or the host and path merge (404). */
    QString normalizedGatewayBase(const QString &endpoint)
    {
        QString e = endpoint.trimmed();
        if (e.isEmpty()) {
            return e;
        }
        if (!e.endsWith(QLatin1Char('/'))) {
            e.append(QLatin1Char('/'));
        }
        return e;
    }
} // namespace

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, QObject *parent)
    : QObject(parent),
      m_gatewayEndpoint(normalizedGatewayBase(gatewayEndpoint)),
      m_isDevEnvironment(isDevEnvironment),
      m_requestTimeoutMsecs(requestTimeoutMsecs),
      m_isStrictKillSwitchEnabled(isStrictKillSwitchEnabled)
{
}

GatewayController::EncryptedRequestData GatewayController::prepareRequest(const QString &endpoint, const QJsonObject &apiPayload)
{
    EncryptedRequestData encRequestData;
    encRequestData.errorCode = ErrorCode::NoError;

#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    encRequestData.request.setTransferTimeout(m_requestTimeoutMsecs);
    encRequestData.request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    encRequestData.request.setRawHeader(QString("X-Client-Request-ID").toUtf8(), QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());
    encRequestData.request.setUrl(endpoint.arg(m_proxyUrl.isEmpty() ? m_gatewayEndpoint : m_proxyUrl));

    // bypass killSwitch exceptions for API-gateway
#ifdef AMNEZIA_DESKTOP
    if (m_isStrictKillSwitchEnabled) {
        QString host = QUrl(encRequestData.request.url()).host();
        QString ip = NetworkUtilities::getIPAddress(host);
        if (!ip.isEmpty()) {
            IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
                QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(QStringList { ip });
                if (!reply.waitForFinished(1000) || !reply.returnValue())
                    qWarning() << "GatewayController::prepareRequest(): Failed to execute remote addKillSwitchAllowedRange call";
            });
        }
    }
#endif

    QSimpleCrypto::QBlockCipher blockCipher;
    encRequestData.key = blockCipher.generatePrivateSalt(32);
    encRequestData.iv = blockCipher.generatePrivateSalt(32);
    encRequestData.salt = blockCipher.generatePrivateSalt(8);

    QJsonObject keyPayload;
    keyPayload[apiDefs::key::aesKey] = QString(encRequestData.key.toBase64());
    keyPayload[apiDefs::key::aesIv] = QString(encRequestData.iv.toBase64());
    keyPayload[apiDefs::key::aesSalt] = QString(encRequestData.salt.toBase64());

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
            encRequestData.errorCode = ErrorCode::ApiMissingAgwPublicKey;
            return encRequestData;
        }

        encryptedKeyPayload = rsa.encrypt(QJsonDocument(keyPayload).toJson(), publicKey, RSA_PKCS1_PADDING);
        EVP_PKEY_free(publicKey);

        encryptedApiPayload = blockCipher.encryptAesBlockCipher(QJsonDocument(apiPayload).toJson(), encRequestData.key, encRequestData.iv,
                                                                "", encRequestData.salt);
    } catch (...) {
        Utils::logException();
        qCritical() << "error when encrypting the request body";
        encRequestData.errorCode = ErrorCode::ApiConfigDecryptionError;
        return encRequestData;
    }

    QJsonObject requestBody;
    requestBody[apiDefs::key::keyPayload] = QString(encryptedKeyPayload.toBase64());
    requestBody[apiDefs::key::apiPayload] = QString(encryptedApiPayload.toBase64());

    encRequestData.requestBody = QJsonDocument(requestBody).toJson();
    return encRequestData;
}

GatewayController::DecryptionResult GatewayController::tryDecryptResponseBody(const QByteArray &encryptedResponseBody,
                                                                              QNetworkReply::NetworkError replyError, const QByteArray &key,
                                                                              const QByteArray &iv, const QByteArray &salt)
{
    Q_UNUSED(replyError);

    DecryptionResult result;
    result.decryptedBody = encryptedResponseBody;
    result.isDecryptionSuccessful = false;

    try {
        QSimpleCrypto::QBlockCipher blockCipher;
        result.decryptedBody = blockCipher.decryptAesBlockCipher(encryptedResponseBody, key, iv, "", salt);
        result.isDecryptionSuccessful = true;
    } catch (...) {
        result.decryptedBody = encryptedResponseBody;
        result.isDecryptionSuccessful = false;
    }

    return result;
}

GatewayController::DecryptionResult GatewayController::resolveResponseBody(const QByteArray &responseBody,
                                                                           QNetworkReply::NetworkError replyError, const QByteArray &key,
                                                                           const QByteArray &iv, const QByteArray &salt)
{
    DecryptionResult result = tryDecryptResponseBody(responseBody, replyError, key, iv, salt);
    if (result.isDecryptionSuccessful || !m_isDevEnvironment) {
        return result;
    }

    const QByteArray trimmed = responseBody.trimmed();
    if (trimmed.isEmpty() || trimmed.front() != '{') {
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        result.decryptedBody = trimmed;
        result.isDecryptionSuccessful = true;
    }
    return result;
}

ErrorCode GatewayController::post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    EncryptedRequestData encRequestData = prepareRequest(endpoint, apiPayload);
    if (encRequestData.errorCode != ErrorCode::NoError) {
        return encRequestData.errorCode;
    }

    QNetworkReply *reply = amnApp->networkManager()->post(encRequestData.request, encRequestData.requestBody);

    QEventLoop wait;
    connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);

    QList<QSslError> sslErrors;
    connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
    execNetworkWaitLoop(wait);

    QByteArray encryptedResponseBody = reply->readAll();
    QString replyErrorString = reply->errorString();
    auto replyError = reply->error();
    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    reply->deleteLater();

    if (encRequestData.isPlaintextLocalGateway) {
        const auto errorCode =
                apiUtils::checkNetworkReplyErrors(sslErrors, replyErrorString, replyError, httpStatusCode, encryptedResponseBody);
        if (errorCode) {
            return errorCode;
        }
        responseBody = encryptedResponseBody;
        return ErrorCode::NoError;
    }

    auto decryptionResult =
            resolveResponseBody(encryptedResponseBody, replyError, encRequestData.key, encRequestData.iv, encRequestData.salt);

    if (sslErrors.isEmpty() && shouldBypassProxy(replyError, decryptionResult.decryptedBody, decryptionResult.isDecryptionSuccessful)) {
        auto requestFunction = [&encRequestData, &encryptedResponseBody](const QString &url) {
            encRequestData.request.setUrl(url);
            return amnApp->networkManager()->post(encRequestData.request, encRequestData.requestBody);
        };

        auto replyProcessingFunction = [&encryptedResponseBody, &replyErrorString, &replyError, &httpStatusCode, &sslErrors, &encRequestData,
                                        &decryptionResult, this](QNetworkReply *reply, const QList<QSslError> &nestedSslErrors) {
            encryptedResponseBody = reply->readAll();
            replyErrorString = reply->errorString();
            replyError = reply->error();
            httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            decryptionResult =
                    resolveResponseBody(encryptedResponseBody, replyError, encRequestData.key, encRequestData.iv, encRequestData.salt);

            if (!sslErrors.isEmpty()
                || shouldBypassProxy(replyError, decryptionResult.decryptedBody, decryptionResult.isDecryptionSuccessful)) {
                sslErrors = nestedSslErrors;
                return false;
            }
            return true;
        };

        auto serviceType = apiPayload.value(apiDefs::key::serviceType).toString("");
        auto userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString("");
        bypassProxy(endpoint, serviceType, userCountryCode, requestFunction, replyProcessingFunction);
    }

    auto errorCode =
            apiUtils::checkNetworkReplyErrors(sslErrors, replyErrorString, replyError, httpStatusCode, decryptionResult.decryptedBody);
    if (errorCode) {
        return errorCode;
    }

    if (!decryptionResult.isDecryptionSuccessful) {
        qCritical() << "error when decrypting the request body";
        return ErrorCode::ApiConfigDecryptionError;
    }

    responseBody = decryptionResult.decryptedBody;
    return ErrorCode::NoError;
}

QFuture<QPair<ErrorCode, QByteArray>> GatewayController::postAsync(const QString &endpoint, const QJsonObject &apiPayload,
                                                                   QNetworkReply **activeReplyOut,
                                                                   const QSharedPointer<GatewayController> &keepAlive)
{
    auto promise = QSharedPointer<QPromise<QPair<ErrorCode, QByteArray>>>::create();
    promise->start();

    const QSharedPointer<GatewayController> life = keepAlive;

    EncryptedRequestData encRequestData = prepareRequest(endpoint, apiPayload);
    if (encRequestData.errorCode != ErrorCode::NoError) {
        promise->addResult(qMakePair(encRequestData.errorCode, QByteArray()));
        promise->finish();
        return promise->future();
    }

    QNetworkReply *reply = amnApp->networkManager()->post(encRequestData.request, encRequestData.requestBody);
    if (activeReplyOut) {
        *activeReplyOut = reply;
    }

    auto sslErrors = QSharedPointer<QList<QSslError>>::create();

    connect(reply, &QNetworkReply::sslErrors, [sslErrors](const QList<QSslError> &errors) { *sslErrors = errors; });

    connect(reply, &QNetworkReply::finished, reply, [promise, sslErrors, encRequestData, endpoint, apiPayload, reply, life]() mutable {
        if (!life) {
            promise->addResult(qMakePair(ErrorCode::ApiConfigDecryptionError, QByteArray()));
            promise->finish();
            return;
        }

        GatewayController *const ctl = life.data();
        QByteArray encryptedResponseBody = reply->readAll();
        QString replyErrorString = reply->errorString();
        auto replyError = reply->error();
        int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        reply->deleteLater();

        if (encRequestData.isPlaintextLocalGateway) {
            const auto errorCode = apiUtils::checkNetworkReplyErrors(*sslErrors, replyErrorString, replyError, httpStatusCode,
                                                                    encryptedResponseBody);
            if (errorCode) {
                promise->addResult(qMakePair(errorCode, QByteArray()));
            } else {
                promise->addResult(qMakePair(ErrorCode::NoError, encryptedResponseBody));
            }
            promise->finish();
            return;
        }

        auto decryptionResult =
                ctl->resolveResponseBody(encryptedResponseBody, replyError, encRequestData.key, encRequestData.iv, encRequestData.salt);

        auto processResponse = [promise, encRequestData](const GatewayController::DecryptionResult &decryptionResult,
                                                         const QList<QSslError> &sslErrors, QNetworkReply::NetworkError replyError,
                                                         const QString &replyErrorString, int httpStatusCode) {
            auto errorCode = apiUtils::checkNetworkReplyErrors(sslErrors, replyErrorString, replyError, httpStatusCode,
                                                               decryptionResult.decryptedBody);
            if (errorCode) {
                promise->addResult(qMakePair(errorCode, QByteArray()));
                promise->finish();
                return;
            }

            if (!decryptionResult.isDecryptionSuccessful) {
                Utils::logException();
                qCritical() << "error when decrypting the request body";
                promise->addResult(qMakePair(ErrorCode::ApiConfigDecryptionError, QByteArray()));
                promise->finish();
                return;
            }

            promise->addResult(qMakePair(ErrorCode::NoError, decryptionResult.decryptedBody));
            promise->finish();
        };

        if (sslErrors->isEmpty()
            && ctl->shouldBypassProxy(replyError, decryptionResult.decryptedBody, decryptionResult.isDecryptionSuccessful)) {
            auto serviceType = apiPayload.value(apiDefs::key::serviceType).toString("");
            auto userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString("");

            QStringList primaryBaseUrls;
            QStringList fallbackBaseUrls;
            if (ctl->m_isDevEnvironment) {
                primaryBaseUrls = QString(DEV_S3_ENDPOINT).split(", ", Qt::SkipEmptyParts);
            } else {
                primaryBaseUrls = QString(PROD_S3_ENDPOINT).split(", ", Qt::SkipEmptyParts);
                fallbackBaseUrls = QString(FALLBACK_S3_ENDPOINT).split(", ", Qt::SkipEmptyParts);
            }
            std::random_device randomDevice;
            std::mt19937 generator(randomDevice());
            std::shuffle(primaryBaseUrls.begin(), primaryBaseUrls.end(), generator);
            std::shuffle(fallbackBaseUrls.begin(), fallbackBaseUrls.end(), generator);

            auto appendStorageUrls = [&serviceType, &userCountryCode](const QStringList &baseUrls, QStringList &target) {
                if (!serviceType.isEmpty()) {
                    for (const auto &baseUrl : baseUrls) {
                        QByteArray path = ("endpoints-" + serviceType + "-" + userCountryCode).toUtf8();
                        target.push_back(baseUrl + path.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals) + ".json");
                    }
                }
                for (const auto &baseUrl : baseUrls) {
                    target.push_back(baseUrl + "endpoints.json");
                }
            };

            QStringList proxyStorageUrls;
            appendStorageUrls(primaryBaseUrls, proxyStorageUrls);
            appendStorageUrls(fallbackBaseUrls, proxyStorageUrls);

            life->getProxyUrlsAsync(life, proxyStorageUrls, 0,
                                    [life, encRequestData, endpoint, processResponse](const QStringList &proxyUrls) {
                                        life->getProxyUrlAsync(life, proxyUrls, 0,
                                                               [life, encRequestData, endpoint, processResponse](
                                                                       const QString &proxyUrl) {
                                                                   life->bypassProxyAsync(
                                                                           life, endpoint, proxyUrl, encRequestData,
                                                                           [processResponse](const QByteArray &decryptedBody,
                                                                                             bool isDecryptionSuccessful,
                                                                                             const QList<QSslError> &sslErrors,
                                                                                             QNetworkReply::NetworkError replyError,
                                                                                             const QString &replyErrorString,
                                                                                             int httpStatusCode) {
                                                                               GatewayController::DecryptionResult result;
                                                                               result.decryptedBody = decryptedBody;
                                                                               result.isDecryptionSuccessful = isDecryptionSuccessful;
                                                                               processResponse(result, sslErrors, replyError,
                                                                                               replyErrorString, httpStatusCode);
                                                                           });
                                                               });
                                    });

        } else {
            processResponse(decryptionResult, *sslErrors, replyError, replyErrorString, httpStatusCode);
        }
    });

    return promise->future();
}

QStringList GatewayController::getProxyUrls(const QString &serviceType, const QString &userCountryCode)
{
    QNetworkRequest request;
    request.setTransferTimeout(proxyStorageRequestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QEventLoop wait;
    QList<QSslError> sslErrors;
    QNetworkReply *reply;

    QStringList primaryBaseUrls;
    QStringList fallbackBaseUrls;
    if (m_isDevEnvironment) {
        primaryBaseUrls = QString(DEV_S3_ENDPOINT).split(", ", Qt::SkipEmptyParts);
    } else {
        primaryBaseUrls = QString(PROD_S3_ENDPOINT).split(", ", Qt::SkipEmptyParts);
        fallbackBaseUrls = QString(FALLBACK_S3_ENDPOINT).split(", ", Qt::SkipEmptyParts);
    }

    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::shuffle(primaryBaseUrls.begin(), primaryBaseUrls.end(), generator);
    std::shuffle(fallbackBaseUrls.begin(), fallbackBaseUrls.end(), generator);

    QByteArray key = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;

    auto appendStorageUrls = [&serviceType, &userCountryCode](const QStringList &baseUrls, QStringList &target) {
        if (!serviceType.isEmpty()) {
            for (const auto &baseUrl : baseUrls) {
                QByteArray path = ("endpoints-" + serviceType + "-" + userCountryCode).toUtf8();
                target.push_back(baseUrl + path.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals) + ".json");
            }
        }
        for (const auto &baseUrl : baseUrls) {
            target.push_back(baseUrl + "endpoints.json");
        }
    };

    QStringList proxyStorageUrls;
    appendStorageUrls(primaryBaseUrls, proxyStorageUrls);
    appendStorageUrls(fallbackBaseUrls, proxyStorageUrls);

    if (proxyStorageUrls.empty()) {
        qDebug() << "empty storage endpoint list";
        return {};
    }

    for (const auto &proxyStorageUrl : proxyStorageUrls) {
        request.setUrl(proxyStorageUrl);
        reply = amnApp->networkManager()->get(request);

        connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
        connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
        execNetworkWaitLoop(wait);

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
            auto replyError = reply->error();
            int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qDebug() << replyError;
            qDebug() << httpStatusCode;
            qDebug() << "go to the next storage endpoint";

            reply->deleteLater();
        }
    }
    return {};
}

bool GatewayController::shouldBypassProxy(const QNetworkReply::NetworkError &replyError, const QByteArray &decryptedResponseBody,
                                          bool isDecryptionSuccessful)
{
    // Dev AGW is reached directly; S3 proxy rotation returns 500 and masks the real error (see pairing long-poll).
    if (m_isDevEnvironment) {
        return false;
    }

    const QByteArray &responseBody = decryptedResponseBody;

    int apiHttpStatus = -1;
    QString apiErrorMessage;
    if (isDecryptionSuccessful) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseBody);
        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            apiHttpStatus = jsonObj.value("http_status").toInt(-1);
            apiErrorMessage = jsonObj.value(QStringLiteral("message")).toString().trimmed();
        }
    } else {
        qDebug() << "failed to decrypt the data";
        return true;
    }

    if (replyError == QNetworkReply::NetworkError::OperationCanceledError || replyError == QNetworkReply::NetworkError::TimeoutError) {
        qDebug() << "timeout occurred";
        qDebug() << replyError;
        return true;
    } 
    if (responseBody.contains("html")) {
        qDebug() << "the response contains an html tag";
        return true;
    } 
    if (apiHttpStatus == httpStatusCodeNotFound) {
        if (responseBody.contains(errorResponsePattern1) || responseBody.contains(errorResponsePattern2)
            || responseBody.contains(errorResponsePattern3)) {
            return false;
        } else {
            qDebug() << replyError;
            return true;
        }
    } 
    if (apiHttpStatus == httpStatusCodeNotImplemented) {
        if (responseBody.contains(updateRequestResponsePattern)) {
            return false;
        } else {
            qDebug() << replyError;
            return true;
        }
    } 
    if (apiHttpStatus == httpStatusCodeConflict) {
        return false;
    } 
    if (apiHttpStatus == httpStatusCodePaymentRequired) {
        return false;
    } 
    if (apiHttpStatus == httpStatusCodeUnprocessableEntity) {
        return apiErrorMessage != unprocessableSubscriptionMessage;
    } 
    if (replyError != QNetworkReply::NetworkError::NoError) {
        qDebug() << replyError;
        return true;
    }
    return false;
}

void GatewayController::bypassProxy(const QString &endpoint, const QString &serviceType, const QString &userCountryCode,
                                    std::function<QNetworkReply *(const QString &url)> requestFunction,
                                    std::function<bool(QNetworkReply *reply, const QList<QSslError> &sslErrors)> replyProcessingFunction)
{
    QStringList proxyUrls = getProxyUrls(serviceType, userCountryCode);
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::shuffle(proxyUrls.begin(), proxyUrls.end(), generator);

    QByteArray responseBody;

    auto bypassFunction = [this](const QString &endpoint, const QString &proxyUrl,
                                 std::function<QNetworkReply *(const QString &url)> requestFunction,
                                 std::function<bool(QNetworkReply * reply, const QList<QSslError> &sslErrors)> replyProcessingFunction) {
        QEventLoop wait;
        QList<QSslError> sslErrors;

        qDebug() << "go to the next proxy endpoint";
        QNetworkReply *reply = requestFunction(endpoint.arg(proxyUrl));

        QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
        connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
        execNetworkWaitLoop(wait);

        auto result = replyProcessingFunction(reply, sslErrors);
        reply->deleteLater();
        return result;
    };

    if (m_proxyUrl.isEmpty()) {
        QNetworkRequest request;
        request.setTransferTimeout(1000);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QEventLoop wait;
        QList<QSslError> sslErrors;
        QNetworkReply *reply;

        for (const QString &proxyUrl : proxyUrls) {
            request.setUrl(proxyUrl + "lmbd-health");
            reply = amnApp->networkManager()->get(request);

            connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
            connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
            execNetworkWaitLoop(wait);

            if (reply->error() == QNetworkReply::NetworkError::NoError) {
                reply->deleteLater();

                m_proxyUrl = proxyUrl;
                if (!m_proxyUrl.isEmpty()) {
                    break;
                }
            } else {
                reply->deleteLater();
            }
        }
    }

    if (!m_proxyUrl.isEmpty()) {
        if (bypassFunction(endpoint, m_proxyUrl, requestFunction, replyProcessingFunction)) {
            return;
        }
    }

    for (const QString &proxyUrl : proxyUrls) {
        if (bypassFunction(endpoint, proxyUrl, requestFunction, replyProcessingFunction)) {
            m_proxyUrl = proxyUrl;
            break;
        }
    }
}

void GatewayController::getProxyUrlsAsync(const QSharedPointer<GatewayController> &life, const QStringList &proxyStorageUrls,
                                          const int currentProxyStorageIndex, const std::function<void(const QStringList &)> &onComplete)
{
    if (!life) {
        onComplete({});
        return;
    }

    if (currentProxyStorageIndex >= proxyStorageUrls.size()) {
        onComplete({});
        return;
    }

    QNetworkRequest request;
    request.setTransferTimeout(proxyStorageRequestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(proxyStorageUrls[currentProxyStorageIndex]);

    QNetworkReply *reply = amnApp->networkManager()->get(request);

    connect(reply, &QNetworkReply::finished, reply, [life, proxyStorageUrls, currentProxyStorageIndex, onComplete, reply]() {
        if (!life) {
            onComplete({});
            reply->deleteLater();
            return;
        }

        GatewayController *const ctl = life.data();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray encrypted = reply->readAll();
            reply->deleteLater();

            QByteArray responseBody;
            try {
                QByteArray key = ctl->m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
                if (!ctl->m_isDevEnvironment) {
                    QCryptographicHash hash(QCryptographicHash::Sha512);
                    hash.addData(key);
                    QByteArray h = hash.result().toHex();

                    QByteArray decKey = QByteArray::fromHex(h.left(64));
                    QByteArray iv = QByteArray::fromHex(h.mid(64, 32));
                    QByteArray ba = QByteArray::fromBase64(encrypted);

                    QSimpleCrypto::QBlockCipher cipher;
                    responseBody = cipher.decryptAesBlockCipher(ba, decKey, iv);
                } else {
                    responseBody = encrypted;
                }
            } catch (...) {
                Utils::logException();
                qCritical() << "error decrypting payload";
                QTimer::singleShot(0, ctl, [life, proxyStorageUrls, currentProxyStorageIndex, onComplete]() {
                    if (life) {
                        life->getProxyUrlsAsync(life, proxyStorageUrls, currentProxyStorageIndex + 1, onComplete);
                    } else {
                        onComplete({});
                    }
                });
                return;
            }

            QJsonArray endpointsArray = QJsonDocument::fromJson(responseBody).array();
            QStringList endpoints;
            for (const QJsonValue &endpoint : endpointsArray) {
                endpoints.push_back(endpoint.toString());
            }

            QStringList shuffled = endpoints;
            std::random_device randomDevice;
            std::mt19937 generator(randomDevice());
            std::shuffle(shuffled.begin(), shuffled.end(), generator);

            onComplete(shuffled);
            return;
        }

        int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << httpStatusCode;
        qDebug() << "go to the next storage endpoint";
        reply->deleteLater();
        QTimer::singleShot(0, ctl, [life, proxyStorageUrls, currentProxyStorageIndex, onComplete]() {
            if (life) {
                life->getProxyUrlsAsync(life, proxyStorageUrls, currentProxyStorageIndex + 1, onComplete);
            } else {
                onComplete({});
            }
        });
    });
}

void GatewayController::getProxyUrlAsync(const QSharedPointer<GatewayController> &life, const QStringList &proxyUrls,
                                         const int currentProxyIndex, const std::function<void(const QString &)> &onComplete)
{
    if (!life) {
        onComplete(QString());
        return;
    }

    if (currentProxyIndex >= proxyUrls.size()) {
        onComplete(QString());
        return;
    }

    QNetworkRequest request;
    request.setTransferTimeout(1000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(proxyUrls[currentProxyIndex] + "lmbd-health");

    QNetworkReply *reply = amnApp->networkManager()->get(request);

    connect(reply, &QNetworkReply::finished, reply, [life, proxyUrls, currentProxyIndex, onComplete, reply]() {
        reply->deleteLater();

        if (!life) {
            onComplete(QString());
            return;
        }

        GatewayController *const ctl = life.data();

        if (reply->error() == QNetworkReply::NoError) {
            m_proxyUrl = proxyUrls[currentProxyIndex];
            onComplete(m_proxyUrl);
            return;
        }

        qDebug() << "go to the next proxy endpoint";
        QTimer::singleShot(0, ctl, [life, proxyUrls, currentProxyIndex, onComplete]() {
            if (life) {
                life->getProxyUrlAsync(life, proxyUrls, currentProxyIndex + 1, onComplete);
            } else {
                onComplete(QString());
            }
        });
    });
}

void GatewayController::bypassProxyAsync(
        const QSharedPointer<GatewayController> &life, const QString &endpoint, const QString &proxyUrl,
        const EncryptedRequestData &encRequestData,
        const std::function<void(const QByteArray &, bool, const QList<QSslError> &, QNetworkReply::NetworkError, const QString &, int)>
                &onComplete)
{
    auto sslErrors = QSharedPointer<QList<QSslError>>::create();
    if (!life) {
        onComplete(QByteArray(), false, *sslErrors, QNetworkReply::InternalServerError, QStringLiteral("gateway gone"), 0);
        return;
    }

    if (proxyUrl.isEmpty()) {
        onComplete(QByteArray(), false, *sslErrors, QNetworkReply::InternalServerError, "empty proxy url", 0);
        return;
    }

    QNetworkRequest request = encRequestData.request;
    request.setUrl(endpoint.arg(proxyUrl));

    QNetworkReply *reply = amnApp->networkManager()->post(request, encRequestData.requestBody);

    connect(reply, &QNetworkReply::sslErrors, reply, [sslErrors](const QList<QSslError> &errors) { *sslErrors = errors; });

    connect(reply, &QNetworkReply::finished, reply, [life, sslErrors, onComplete, encRequestData, reply]() {
        QByteArray encryptedResponseBody = reply->readAll();
        QString replyErrorString = reply->errorString();
        auto replyError = reply->error();
        int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        reply->deleteLater();

        if (!life) {
            onComplete(QByteArray(), false, *sslErrors, QNetworkReply::InternalServerError, QStringLiteral("gateway gone"), 0);
            return;
        }

        auto decryptionResult = life->resolveResponseBody(encryptedResponseBody, replyError, encRequestData.key, encRequestData.iv,
                                                            encRequestData.salt);

        onComplete(decryptionResult.decryptedBody, decryptionResult.isDecryptionSuccessful, *sslErrors, replyError, replyErrorString,
                   httpStatusCode);
    });
}
