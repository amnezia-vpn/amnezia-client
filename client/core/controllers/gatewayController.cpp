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
#include <QUrl>

#include <openssl/rsa.h>

#include "amneziaApplication.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/networkUtilities.h"
#include "cryptoUtils.h"

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
#endif

namespace
{
    constexpr QLatin1String errorResponsePattern1("No active configuration found for");
    constexpr QLatin1String errorResponsePattern2("No non-revoked public key found for");
    constexpr QLatin1String errorResponsePattern3("Account not found.");
    constexpr QLatin1String errorResponsePatternQrSessionNotFound("QR session not found");
    constexpr QLatin1String errorResponsePatternSessionNotFound("Session not found");

    constexpr QLatin1String updateRequestResponsePattern("client version update is required");

    constexpr int httpStatusCodeNotFound = 404;
    constexpr int httpStatusCodeConflict = 409;
    constexpr int httpStatusCodeNotImplemented = 501;
    constexpr int httpStatusCodePaymentRequired = 402;
    constexpr int httpStatusCodeRequestTimeout = 408;
    constexpr int httpStatusCodeUnprocessableEntity = 422;

    constexpr QLatin1String unprocessableSubscriptionMessage("Failed to retrieve subscription information. Is it activated?");

    constexpr int proxyStorageRequestTimeoutMsecs = 3000;

    QStringList shuffledProxyUrls(const QStringList &proxyUrls)
    {
        QStringList shuffled = proxyUrls;
        std::random_device randomDevice;
        std::mt19937 generator(randomDevice());
        std::shuffle(shuffled.begin(), shuffled.end(), generator);
        return shuffled;
    }

    QString getProxyUrlsCacheKey(const QString &serviceType, const QString &userCountryCode)
    {
        return QStringLiteral("service_%1_country_%2").arg(serviceType, userCountryCode);
    }

    bool decryptProxyUrlsPayload(const QByteArray &encryptedPayload, bool isDevEnvironment, QByteArray &decryptedPayload)
    {
        QByteArray key = isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
        if (!isDevEnvironment) {
            QCryptographicHash hash(QCryptographicHash::Sha512);
            hash.addData(key);
            QByteArray h = hash.result().toHex();

            QByteArray decKey = QByteArray::fromHex(h.left(64));
            QByteArray iv = QByteArray::fromHex(h.mid(64, 32));
            QByteArray ba = QByteArray::fromBase64(encryptedPayload);

            decryptedPayload = CryptoUtils::decryptAes256Cbc(ba, decKey, iv);
            if (decryptedPayload.isEmpty()) {
                return false;
            }
        } else {
            decryptedPayload = encryptedPayload;
        }
        return true;
    }

    QStringList readCachedProxyUrls(const QByteArray &cachedProxyUrlsEncrypted, bool isDevEnvironment)
    {
        if (cachedProxyUrlsEncrypted.isEmpty()) {
            return {};
        }

        QByteArray cachedProxyUrlsDecrypted;
        if (!decryptProxyUrlsPayload(cachedProxyUrlsEncrypted, isDevEnvironment, cachedProxyUrlsDecrypted)) {
            qCritical() << "error decrypting cached proxy urls payload";
            return {};
        }

        QJsonArray endpointsArray = QJsonDocument::fromJson(cachedProxyUrlsDecrypted).array();
        QStringList endpoints;
        endpoints.reserve(endpointsArray.size());
        for (const QJsonValue &endpoint : endpointsArray) {
            endpoints.push_back(endpoint.toString());
        }

        return endpoints;
    }
}

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                                     QObject *parent)
    : QObject(parent),
      m_gatewayEndpoint(gatewayEndpoint),
      m_isDevEnvironment(isDevEnvironment),
      m_requestTimeoutMsecs(requestTimeoutMsecs),
      m_isStrictKillSwitchEnabled(isStrictKillSwitchEnabled),
      m_appSettingsRepository(appSettingsRepository)
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

    encRequestData.key = CryptoUtils::generateRandomBytes(32);
    encRequestData.iv = CryptoUtils::generateRandomBytes(32);
    encRequestData.salt = CryptoUtils::generateRandomBytes(8);

    QJsonObject keyPayload;
    keyPayload[apiDefs::key::aesKey] = QString(encRequestData.key.toBase64());
    keyPayload[apiDefs::key::aesIv] = QString(encRequestData.iv.toBase64());
    keyPayload[apiDefs::key::aesSalt] = QString(encRequestData.salt.toBase64());

    QByteArray rsaKey = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
    EVP_PKEY *publicKey = CryptoUtils::loadPublicKeyFromPem(rsaKey);
    if (publicKey == nullptr) {
        qCritical() << "error loading public key from environment variables";
        encRequestData.errorCode = ErrorCode::ApiMissingAgwPublicKey;
        return encRequestData;
    }

    QByteArray encryptedKeyPayload = CryptoUtils::rsaEncrypt(QJsonDocument(keyPayload).toJson(), publicKey, RSA_PKCS1_PADDING);
    EVP_PKEY_free(publicKey);

    QByteArray encryptedApiPayload = CryptoUtils::encryptAes256Cbc(QJsonDocument(apiPayload).toJson(), encRequestData.key, encRequestData.iv);

    if (encryptedKeyPayload.isEmpty() || encryptedApiPayload.isEmpty()) {
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
    DecryptionResult result;
    result.decryptedBody = encryptedResponseBody;
    result.isDecryptionSuccessful = false;

    QByteArray decrypted = CryptoUtils::decryptAes256Cbc(encryptedResponseBody, key, iv);
    if (!decrypted.isEmpty()) {
        result.decryptedBody = decrypted;
        result.isDecryptionSuccessful = true;
    } else {
        result.decryptedBody = encryptedResponseBody;
        result.isDecryptionSuccessful = false;
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
    wait.exec(QEventLoop::ExcludeUserInputEvents);

    QByteArray encryptedResponseBody = reply->readAll();
    QString replyErrorString = reply->errorString();
    auto replyError = reply->error();
    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    reply->deleteLater();

    auto decryptionResult =
            tryDecryptResponseBody(encryptedResponseBody, replyError, encRequestData.key, encRequestData.iv, encRequestData.salt);

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
                    tryDecryptResponseBody(encryptedResponseBody, replyError, encRequestData.key, encRequestData.iv, encRequestData.salt);

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

    responseBody = decryptionResult.decryptedBody;
    const auto errorCode =
            apiUtils::checkNetworkReplyErrors(sslErrors, replyErrorString, replyError, httpStatusCode, responseBody);
    if (errorCode) {
        return errorCode;
    }

    if (!decryptionResult.isDecryptionSuccessful) {
        qCritical() << "error when decrypting the request body";
        return ErrorCode::ApiConfigDecryptionError;
    }

    return ErrorCode::NoError;
}

QFuture<QPair<ErrorCode, QByteArray>> GatewayController::postAsync(const QString &endpoint, const QJsonObject apiPayload)
{
    auto promise = QSharedPointer<QPromise<QPair<ErrorCode, QByteArray>>>::create();
    promise->start();

    EncryptedRequestData encRequestData = prepareRequest(endpoint, apiPayload);
    if (encRequestData.errorCode != ErrorCode::NoError) {
        promise->addResult(qMakePair(encRequestData.errorCode, QByteArray()));
        promise->finish();
        return promise->future();
    }

    QNetworkReply *reply = amnApp->networkManager()->post(encRequestData.request, encRequestData.requestBody);

    auto sslErrors = QSharedPointer<QList<QSslError>>::create();

    connect(reply, &QNetworkReply::sslErrors, [sslErrors](const QList<QSslError> &errors) { *sslErrors = errors; });

    connect(reply, &QNetworkReply::finished, this, [promise, sslErrors, encRequestData, endpoint, apiPayload, reply, this]() mutable {
        QByteArray encryptedResponseBody = reply->readAll();
        QString replyErrorString = reply->errorString();
        auto replyError = reply->error();
        int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        reply->deleteLater();

        auto decryptionResult =
                tryDecryptResponseBody(encryptedResponseBody, replyError, encRequestData.key, encRequestData.iv, encRequestData.salt);

        auto processResponse = [promise, encRequestData](const GatewayController::DecryptionResult &decryptionResult,
                                                         const QList<QSslError> &sslErrors, QNetworkReply::NetworkError replyError,
                                                         const QString &replyErrorString, int httpStatusCode) {
            auto errorCode = apiUtils::checkNetworkReplyErrors(sslErrors, replyErrorString, replyError, httpStatusCode,
                                                               decryptionResult.decryptedBody);
            if (errorCode) {
                promise->addResult(qMakePair(errorCode, decryptionResult.decryptedBody));
                promise->finish();
                return;
            }

            if (!decryptionResult.isDecryptionSuccessful) {
                qCritical() << "error when decrypting the request body";
                promise->addResult(qMakePair(ErrorCode::ApiConfigDecryptionError, QByteArray()));
                promise->finish();
                return;
            }

            promise->addResult(qMakePair(ErrorCode::NoError, decryptionResult.decryptedBody));
            promise->finish();
        };

        if (sslErrors->isEmpty() && shouldBypassProxy(replyError, decryptionResult.decryptedBody, decryptionResult.isDecryptionSuccessful)) {
            auto serviceType = apiPayload.value(apiDefs::key::serviceType).toString("");
            auto userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString("");

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
            const QString proxyUrlsCacheKey = getProxyUrlsCacheKey(serviceType, userCountryCode);

            getProxyUrlsAsync(proxyStorageUrls, 0, proxyUrlsCacheKey, [this, encRequestData, endpoint, processResponse](const QStringList &proxyUrls) {
                getProxyUrlAsync(proxyUrls, 0, [this, encRequestData, endpoint, processResponse](const QString &proxyUrl) {
                    bypassProxyAsync(endpoint, proxyUrl, encRequestData,
                                     [processResponse, this](const QByteArray &decryptedBody, bool isDecryptionSuccessful,
                                                             const QList<QSslError> &sslErrors, QNetworkReply::NetworkError replyError,
                                                             const QString &replyErrorString, int httpStatusCode) {
                                         GatewayController::DecryptionResult result;
                                         result.decryptedBody = decryptedBody;
                                         result.isDecryptionSuccessful = isDecryptionSuccessful;
                                         processResponse(result, sslErrors, replyError, replyErrorString, httpStatusCode);
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
    const QString proxyUrlsCacheKey = getProxyUrlsCacheKey(serviceType, userCountryCode);
    const QByteArray cachedProxyUrlsEncrypted = m_appSettingsRepository->readGatewayProxyUrls(proxyUrlsCacheKey);

    if (proxyStorageUrls.empty()) {
        qDebug() << "empty storage endpoint list";
        return readCachedProxyUrls(cachedProxyUrlsEncrypted, m_isDevEnvironment);
    }

    for (const auto &proxyStorageUrl : proxyStorageUrls) {
        request.setUrl(proxyStorageUrl);
        reply = amnApp->networkManager()->get(request);

        connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
        connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
        wait.exec(QEventLoop::ExcludeUserInputEvents);

        if (reply->error() == QNetworkReply::NetworkError::NoError) {
            auto encryptedResponseBody = reply->readAll();
            reply->deleteLater();

            QByteArray responseBody;
            if (!decryptProxyUrlsPayload(encryptedResponseBody, m_isDevEnvironment, responseBody)) {
                qCritical() << "error loading private key from environment variables or decrypting payload" << encryptedResponseBody;
                continue;
            }

            auto endpointsArray = QJsonDocument::fromJson(responseBody).array();

            QStringList endpoints;
            for (const auto &endpoint : endpointsArray) {
                endpoints.push_back(endpoint.toString());
            }
            m_appSettingsRepository->writeGatewayProxyUrls(proxyUrlsCacheKey, encryptedResponseBody);

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
    return readCachedProxyUrls(cachedProxyUrlsEncrypted, m_isDevEnvironment);
}

bool GatewayController::shouldBypassProxy(const QNetworkReply::NetworkError &replyError, const QByteArray &decryptedResponseBody,
                                          bool isDecryptionSuccessful)
{
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
    if (apiHttpStatus == httpStatusCodeRequestTimeout) {
        return false;
    }
    if (apiHttpStatus == httpStatusCodeNotFound) {
        if (responseBody.contains(errorResponsePattern1) || responseBody.contains(errorResponsePattern2)
            || responseBody.contains(errorResponsePattern3) || responseBody.contains(errorResponsePatternQrSessionNotFound)
            || responseBody.contains(errorResponsePatternSessionNotFound)) {
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
        wait.exec(QEventLoop::ExcludeUserInputEvents);

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
            wait.exec(QEventLoop::ExcludeUserInputEvents);

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

void GatewayController::getProxyUrlsAsync(const QStringList proxyStorageUrls, const int currentProxyStorageIndex,
                                          const QString &proxyUrlsCacheKey, std::function<void(const QStringList &)> onComplete)
{
    const QByteArray cachedProxyUrlsEncrypted = m_appSettingsRepository->readGatewayProxyUrls(proxyUrlsCacheKey);

    if (currentProxyStorageIndex >= proxyStorageUrls.size()) {
        onComplete(shuffledProxyUrls(readCachedProxyUrls(cachedProxyUrlsEncrypted, m_isDevEnvironment)));
        return;
    }

    QNetworkRequest request;
    request.setTransferTimeout(proxyStorageRequestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(proxyStorageUrls[currentProxyStorageIndex]);

    QNetworkReply *reply = amnApp->networkManager()->get(request);

    // connect(reply, &QNetworkReply::sslErrors, this, [state](const QList<QSslError> &e) { *(state->sslErrors) = e; });

    connect(reply, &QNetworkReply::finished, this,
            [this, proxyStorageUrls, currentProxyStorageIndex, proxyUrlsCacheKey, onComplete, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray encrypted = reply->readAll();
            reply->deleteLater();

            QByteArray responseBody;
            if (!decryptProxyUrlsPayload(encrypted, m_isDevEnvironment, responseBody)) {
                qCritical() << "error decrypting payload";
                QMetaObject::invokeMethod(
                        this, [=]() { getProxyUrlsAsync(proxyStorageUrls, currentProxyStorageIndex + 1, proxyUrlsCacheKey, onComplete); }, Qt::QueuedConnection);
                return;
            }

            QJsonArray endpointsArray = QJsonDocument::fromJson(responseBody).array();
            QStringList endpoints;
            for (const QJsonValue &endpoint : endpointsArray)
                endpoints.push_back(endpoint.toString());
            m_appSettingsRepository->writeGatewayProxyUrls(proxyUrlsCacheKey, encrypted);

            onComplete(shuffledProxyUrls(endpoints));
            return;
        }

        int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << httpStatusCode;
        qDebug() << "go to the next storage endpoint";
        reply->deleteLater();
        QMetaObject::invokeMethod(
                this, [=]() { getProxyUrlsAsync(proxyStorageUrls, currentProxyStorageIndex + 1, proxyUrlsCacheKey, onComplete); }, Qt::QueuedConnection);
    });
}

void GatewayController::getProxyUrlAsync(const QStringList proxyUrls, const int currentProxyIndex,
                                         std::function<void(const QString &)> onComplete)
{
    if (currentProxyIndex >= proxyUrls.size()) {
        onComplete("");
        return;
    }

    QNetworkRequest request;
    request.setTransferTimeout(1000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(proxyUrls[currentProxyIndex] + "lmbd-health");

    QNetworkReply *reply = amnApp->networkManager()->get(request);

    // connect(reply, &QNetworkReply::sslErrors, this, [state](const QList<QSslError> &e) {
    //     *(state->sslErrors) = e;
    // });

    connect(reply, &QNetworkReply::finished, this, [this, proxyUrls, currentProxyIndex, onComplete, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            m_proxyUrl = proxyUrls[currentProxyIndex];
            onComplete(m_proxyUrl);
            return;
        }

        qDebug() << "go to the next proxy endpoint";
        QMetaObject::invokeMethod(this, [=]() { getProxyUrlAsync(proxyUrls, currentProxyIndex + 1, onComplete); }, Qt::QueuedConnection);
    });
}

void GatewayController::bypassProxyAsync(
        const QString &endpoint, const QString &proxyUrl, EncryptedRequestData encRequestData,
        std::function<void(const QByteArray &, bool, const QList<QSslError> &, QNetworkReply::NetworkError, const QString &, int)> onComplete)
{
    auto sslErrors = QSharedPointer<QList<QSslError>>::create();
    if (proxyUrl.isEmpty()) {
        onComplete(QByteArray(), false, *sslErrors, QNetworkReply::InternalServerError, "empty proxy url", 0);
        return;
    }

    QNetworkRequest request = encRequestData.request;
    request.setUrl(endpoint.arg(proxyUrl));

    QNetworkReply *reply = amnApp->networkManager()->post(request, encRequestData.requestBody);

    connect(reply, &QNetworkReply::sslErrors, this, [sslErrors](const QList<QSslError> &errors) { *sslErrors = errors; });

    connect(reply, &QNetworkReply::finished, this, [sslErrors, onComplete, encRequestData, reply, this]() {
        QByteArray encryptedResponseBody = reply->readAll();
        QString replyErrorString = reply->errorString();
        auto replyError = reply->error();
        int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        reply->deleteLater();

        auto decryptionResult =
                tryDecryptResponseBody(encryptedResponseBody, replyError, encRequestData.key, encRequestData.iv, encRequestData.salt);

        onComplete(decryptionResult.decryptedBody, decryptionResult.isDecryptionSuccessful, *sslErrors, replyError, replyErrorString,
                   httpStatusCode);
    });
}
