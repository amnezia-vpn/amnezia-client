#include "httpGatewayTransport.h"

#include <algorithm>
#include <random>

#include <QCryptographicHash>
#include <QDebug>
#include <QEventLoop>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSharedPointer>
#include <QThread>
#include <QUrl>
#include <QUuid>

#include "QBlockCipher.h"

#include "amnezia_application.h"
#include "core/api/apiUtils.h"
#include "core/networkUtilities.h"
#include "utilities.h"

#ifdef AMNEZIA_DESKTOP
    #include "core/ipcclient.h"
#endif

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

namespace amnezia::transport
{

QMutex HttpGatewayTransport::s_proxyMutex;
QString HttpGatewayTransport::s_proxyUrl;

namespace
{
    constexpr int kProxyHealthTimeoutMsecs = 1000;
    constexpr int httpStatusCodeNotFound = 404;
    constexpr int httpStatusCodeConflict = 409;
    constexpr int httpStatusCodeNotImplemented = 501;

    constexpr QLatin1String errorResponsePattern1("No active configuration found for");
    constexpr QLatin1String errorResponsePattern2("No non-revoked public key found for");
    constexpr QLatin1String errorResponsePattern3("Account not found.");
    constexpr QLatin1String updateRequestResponsePattern("client version update is required");
} // namespace

HttpGatewayTransport::HttpGatewayTransport(const QString &endpoint,
                                           bool isDevEnvironment,
                                           int requestTimeoutMsecs,
                                           bool isStrictKillSwitchEnabled)
    : m_endpoint(endpoint),
      m_isDevEnvironment(isDevEnvironment),
      m_requestTimeoutMsecs(requestTimeoutMsecs),
      m_isStrictKillSwitchEnabled(isStrictKillSwitchEnabled)
{
}

void HttpGatewayTransport::applyKillSwitchAllowlist(const QString &host)
{
#ifdef AMNEZIA_DESKTOP
    if (!m_isStrictKillSwitchEnabled || host.isEmpty()) {
        return;
    }
    const QString ip = NetworkUtilities::getIPAddress(host);
    if (ip.isEmpty()) {
        return;
    }
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(QStringList { ip });
        if (!reply.waitForFinished(1000) || !reply.returnValue()) {
            qWarning() << "HttpGatewayTransport: addKillSwitchAllowedRange failed for" << ip;
        }
    });
#else
    Q_UNUSED(host)
#endif
}

HttpGatewayTransport::ReplyOutcome HttpGatewayTransport::doPost(const QString &fullUrl, const QByteArray &requestBody)
{
    ReplyOutcome outcome;

#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    QNetworkRequest request;
    request.setTransferTimeout(m_requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-Client-Request-ID",
                         QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());
    request.setUrl(fullUrl);

    applyKillSwitchAllowlist(QUrl(fullUrl).host());

    QNetworkReply *reply = amnApp->networkManager()->post(request, requestBody);

    QEventLoop wait;
    QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::sslErrors, [&, reply](const QList<QSslError> &errors) {
        outcome.sslErrors = errors;
#ifdef AGW_INSECURE_SSL
        qWarning() << "[HTTP] sslErrors (ignored, AGW_INSECURE_SSL=1):" << errors;
        reply->ignoreSslErrors();
        outcome.sslErrors.clear();
#endif
    });
    wait.exec(QEventLoop::ExcludeUserInputEvents);

    outcome.encryptedBody = reply->readAll();
    outcome.errorString = reply->errorString();
    outcome.networkError = reply->error();
    outcome.httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    reply->deleteLater();
    return outcome;
}

bool HttpGatewayTransport::shouldBypass(const ReplyOutcome &outcome, const DecryptionResult &decrypted) const
{
    if (!outcome.sslErrors.isEmpty()) {
        return false;
    }

    if (!decrypted.isOk) {
        return true;
    }

    int apiHttpStatus = -1;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(decrypted.decrypted);
    if (jsonDoc.isObject()) {
        apiHttpStatus = jsonDoc.object().value("http_status").toInt(-1);
    }

    if (outcome.networkError == QNetworkReply::NetworkError::OperationCanceledError
        || outcome.networkError == QNetworkReply::NetworkError::TimeoutError) {
        return true;
    }
    if (decrypted.decrypted.contains("html")) {
        return true;
    }
    if (apiHttpStatus == httpStatusCodeNotFound) {
        if (decrypted.decrypted.contains(errorResponsePattern1)
            || decrypted.decrypted.contains(errorResponsePattern2)
            || decrypted.decrypted.contains(errorResponsePattern3)) {
            return false;
        }
        return true;
    }
    if (apiHttpStatus == httpStatusCodeNotImplemented) {
        if (decrypted.decrypted.contains(updateRequestResponsePattern)) {
            return false;
        }
        return true;
    }
    if (apiHttpStatus == httpStatusCodeConflict) {
        return false;
    }
    if (outcome.networkError != QNetworkReply::NetworkError::NoError) {
        return true;
    }
    return false;
}

QStringList HttpGatewayTransport::fetchProxyUrls(const QByteArray &/*serviceHint*/)
{
    QStringList baseUrls = m_isDevEnvironment
            ? QString(DEV_S3_ENDPOINT).split(", ")
            : QString(PROD_S3_ENDPOINT).split(", ");

    QByteArray rsaKey = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;

    QStringList proxyStorageUrls;
    for (const auto &baseUrl : baseUrls) {
        proxyStorageUrls.push_back(baseUrl + "endpoints.json");
    }

    QNetworkRequest request;
    request.setTransferTimeout(m_requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    for (const auto &proxyStorageUrl : proxyStorageUrls) {
        request.setUrl(proxyStorageUrl);
        QNetworkReply *reply = amnApp->networkManager()->get(request);
        QEventLoop wait;
        QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
        wait.exec(QEventLoop::ExcludeUserInputEvents);

        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            continue;
        }

        QByteArray encryptedResponseBody = reply->readAll();
        reply->deleteLater();

        QByteArray responseBody;
        try {
            if (!m_isDevEnvironment) {
                QCryptographicHash hash(QCryptographicHash::Sha512);
                hash.addData(rsaKey);
                QByteArray hashResult = hash.result().toHex();

                QByteArray key = QByteArray::fromHex(hashResult.left(64));
                QByteArray iv = QByteArray::fromHex(hashResult.mid(64, 32));

                QSimpleCrypto::QBlockCipher blockCipher;
                responseBody = blockCipher.decryptAesBlockCipher(QByteArray::fromBase64(encryptedResponseBody), key, iv);
            } else {
                responseBody = encryptedResponseBody;
            }
        } catch (...) {
            Utils::logException();
            qCritical() << "HttpGatewayTransport: error decrypting proxy storage payload";
            continue;
        }

        QJsonArray endpointsArray = QJsonDocument::fromJson(responseBody).array();
        QStringList endpoints;
        endpoints.reserve(endpointsArray.size());
        for (const QJsonValue &endpoint : endpointsArray) {
            endpoints.push_back(endpoint.toString());
        }
        return endpoints;
    }

    return {};
}

amnezia::ErrorCode HttpGatewayTransport::send(const QString &endpointTemplate,
                                              const QByteArray &requestBody,
                                              QByteArray &decryptedResponse,
                                              const DecryptionHook &decryptionHook)
{
    auto buildOutcome = [&](const QString &gatewayBase) {
        return doPost(endpointTemplate.arg(gatewayBase), requestBody);
    };

    auto tryDecrypt = [&](const QByteArray &encrypted) -> DecryptionResult {
        if (!decryptionHook) {
            DecryptionResult r;
            r.decrypted = encrypted;
            r.isOk = false;
            return r;
        }
        return decryptionHook(encrypted);
    };

    QString cachedProxy;
    {
        QMutexLocker lock(&s_proxyMutex);
        cachedProxy = s_proxyUrl;
    }
    const QString primaryBase = cachedProxy.isEmpty() ? m_endpoint : cachedProxy;

    ReplyOutcome outcome = buildOutcome(primaryBase);
    DecryptionResult decrypted = tryDecrypt(outcome.encryptedBody);

    if (outcome.sslErrors.isEmpty() && shouldBypass(outcome, decrypted)) {
        QStringList proxyUrls = fetchProxyUrls(QByteArray());
        std::random_device randomDevice;
        std::mt19937 generator(randomDevice());
        std::shuffle(proxyUrls.begin(), proxyUrls.end(), generator);

        bool bypassResolved = false;

        if (cachedProxy.isEmpty()) {
            QNetworkRequest healthRequest;
            healthRequest.setTransferTimeout(kProxyHealthTimeoutMsecs);
            healthRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

            for (const QString &proxyUrl : std::as_const(proxyUrls)) {
                healthRequest.setUrl(proxyUrl + "lmbd-health");
                QNetworkReply *reply = amnApp->networkManager()->get(healthRequest);
                QEventLoop wait;
                QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
                wait.exec(QEventLoop::ExcludeUserInputEvents);

                const auto err = reply->error();
                reply->deleteLater();
                if (err == QNetworkReply::NoError) {
                    QMutexLocker lock(&s_proxyMutex);
                    s_proxyUrl = proxyUrl;
                    cachedProxy = proxyUrl;
                    break;
                }
            }
        }

        if (!cachedProxy.isEmpty()) {
            ReplyOutcome retry = buildOutcome(cachedProxy);
            DecryptionResult retryDecrypted = tryDecrypt(retry.encryptedBody);
            if (retry.sslErrors.isEmpty() && !shouldBypass(retry, retryDecrypted)) {
                outcome = retry;
                decrypted = retryDecrypted;
                bypassResolved = true;
            }
        }

        if (!bypassResolved) {
            for (const QString &proxyUrl : std::as_const(proxyUrls)) {
                ReplyOutcome retry = buildOutcome(proxyUrl);
                DecryptionResult retryDecrypted = tryDecrypt(retry.encryptedBody);
                if (retry.sslErrors.isEmpty() && !shouldBypass(retry, retryDecrypted)) {
                    {
                        QMutexLocker lock(&s_proxyMutex);
                        s_proxyUrl = proxyUrl;
                    }
                    outcome = retry;
                    decrypted = retryDecrypted;
                    bypassResolved = true;
                    break;
                }
            }
        }
    }

    auto errorCode = apiUtils::checkNetworkReplyErrors(outcome.sslErrors,
                                                       outcome.errorString,
                                                       outcome.networkError,
                                                       outcome.httpStatusCode,
                                                       decrypted.decrypted);
    if (errorCode != amnezia::ErrorCode::NoError) {
        return errorCode;
    }

    if (!decrypted.isOk) {
        qCritical() << "HttpGatewayTransport: response decryption failed";
        return amnezia::ErrorCode::ApiConfigDecryptionError;
    }

    decryptedResponse = decrypted.decrypted;
    return amnezia::ErrorCode::NoError;
}

} // namespace amnezia::transport
