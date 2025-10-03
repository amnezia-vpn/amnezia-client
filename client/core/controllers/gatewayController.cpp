#include "gatewayController.h"

#include <algorithm>
#include <random>

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QDataStream>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QRemoteObjectPendingReply>
#include <QThread>
#include <QUrl>
#include <QtEndian>
#include <QDebug>

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
        const QUrl originalUrl = request.url();
        const QString originalHost = originalUrl.host();
        const QString resolvedIp = allowKillSwitchExceptionForUrl(originalUrl);
        if (!resolvedIp.isEmpty() && resolvedIp != originalHost) {
            QUrl ipUrl = originalUrl;
            ipUrl.setHost(resolvedIp);
            request.setUrl(ipUrl);
            request.setPeerVerifyName(originalHost);
            request.setRawHeader("Host", originalHost.toUtf8());
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
    qDebug() << "apiPayload" << apiPayload;
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    QNetworkRequest request;
    request.setTransferTimeout(m_requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setUrl(endpoint.arg(m_gatewayEndpoint));

    // bypass killSwitch exceptions for API-gateway
    qDebug() << "url" << request.url();
    qDebug() << "host" << QUrl(request.url()).host();
    qDebug() << "endpoint" << endpoint;
#ifdef AMNEZIA_DESKTOP
    if (m_isStrictKillSwitchEnabled) {
        const QUrl originalUrl = request.url();
        const QString originalHost = originalUrl.host();
        const QString resolvedIp = allowKillSwitchExceptionForUrl(originalUrl);
        if (!resolvedIp.isEmpty() && resolvedIp != originalHost) {
            QUrl ipUrl = originalUrl;
            ipUrl.setHost(resolvedIp);
            request.setUrl(ipUrl);
            request.setPeerVerifyName(originalHost);
            request.setRawHeader("Host", originalHost.toUtf8());
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

QString GatewayController::allowKillSwitchExceptionForUrl(const QUrl &url)
{
#ifdef AMNEZIA_DESKTOP
    qDebug() << "allowKillSwitchExceptionForUrl: processing url" << url;
    const QString host = url.host();
    if (host.isEmpty()) {
        qDebug() << "allowKillSwitchExceptionForUrl: empty host, skipping";
        return {};
    }

    qDebug() << "allowKillSwitchExceptionForUrl: resolving host" << host;
    const QString resolvedIp = resolveHost(host);
    if (resolvedIp.isEmpty()) {
        qWarning() << "Failed to resolve host for KillSwitch exception" << host;
        return {};
    }

    qDebug() << "allowKillSwitchExceptionForUrl: adding KillSwitch exception for" << resolvedIp;
    if (!addKillSwitchException(QStringList { resolvedIp })) {
        qWarning() << "Failed to add KillSwitch exception" << resolvedIp;
        return {};
    }
    qDebug() << "allowKillSwitchExceptionForUrl: exception added" << resolvedIp;
    return resolvedIp;
#else
    Q_UNUSED(url);
    return {};
#endif
}

QString GatewayController::resolveHost(const QString &host)
{
#ifdef AMNEZIA_DESKTOP
    qDebug() << "resolveHost: start" << host;
    if (!m_isStrictKillSwitchEnabled) {
        qDebug() << "resolveHost: strict KillSwitch disabled, using NetworkUtilities directly";
        return NetworkUtilities::getIPAddress(host);
    }

    QString resolvedIp = NetworkUtilities::getIPAddress(host);
    qDebug() << "resolveHost: NetworkUtilities returned" << resolvedIp;
    if (!resolvedIp.isEmpty()) {
        return resolvedIp;
    }

    qDebug() << "resolveHost: falling back to resolveHostViaOpenDns" << host;
    resolvedIp = resolveHostViaOpenDns(host);
    if (resolvedIp.isEmpty()) {
        qWarning() << "OpenDNS fallback failed" << host;
    }
    qDebug() << "resolveHost: final result" << resolvedIp;
    return resolvedIp;
#else
    return NetworkUtilities::getIPAddress(host);
#endif
}

#ifdef AMNEZIA_DESKTOP
bool GatewayController::addKillSwitchException(const QStringList &ranges)
{
    qDebug() << "addKillSwitchException: requested ranges" << ranges;
    auto interface = IpcClient::Interface();
    if (!interface) {
        qWarning() << "IPC interface is null, cannot add KillSwitch exception";
        return false;
    }

    const auto waitForReply = [](QRemoteObjectPendingReply<bool> reply) -> bool {
        if (!reply.waitForFinished()) {
            qWarning() << "Timed out waiting for KillSwitch exception reply";
            return false;
        }
        return reply.returnValue();
    };

    QRemoteObjectPendingReply<bool> reply;
    const bool sameThread = interface->thread() == QThread::currentThread();
    qDebug() << "addKillSwitchException: same thread" << sameThread;
    if (interface->thread() == QThread::currentThread()) {
        qDebug() << "addKillSwitchException: invoking directly";
        reply = interface->addKillSwitchAllowedRange(ranges);
    } else {
        qDebug() << "addKillSwitchException: invoking via Qt::BlockingQueuedConnection";
        const bool invoked = QMetaObject::invokeMethod(interface.data(),
                                                       [&reply, interface, ranges]() {
                                                           reply = interface->addKillSwitchAllowedRange(ranges);
                                                       },
                                                       Qt::BlockingQueuedConnection);

        if (!invoked) {
            qWarning() << "Failed to invoke KillSwitch exception update via queued connection";
            return false;
        }
    }

    qDebug() << "addKillSwitchException: waiting for reply";
    const bool result = waitForReply(reply);
    qDebug() << "addKillSwitchException: reply result" << result;
    return result;
}

QString GatewayController::resolveHostViaOpenDns(const QString &host)
{
    qDebug() << "resolveHostViaOpenDns: start" << host;
    const QString fallbackIp = QStringLiteral("146.112.41.2");
    const QString dohHostname = QStringLiteral("doh.opendns.com");
    const QUrl dohEndpoint(QStringLiteral("https://%1/dns-query").arg(fallbackIp));

    if (!addKillSwitchException(QStringList { fallbackIp })) {
        qWarning() << "Failed to add fallback KillSwitch exception" << fallbackIp;
    } else {
        qDebug() << "resolveHostViaOpenDns: fallback KillSwitch exception added" << fallbackIp;
    }

    QNetworkRequest request(dohEndpoint);
    qDebug() << "resolveHostViaOpenDns: DoH endpoint" << dohEndpoint;
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/dns-message"));
    request.setRawHeader("Accept", "application/dns-message");
    request.setRawHeader("Host", dohHostname.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    request.setPeerVerifyName(dohHostname);
    qDebug() << "resolveHostViaOpenDns: peer verify name set" << dohHostname;

    QByteArray payload = buildDnsQuery(host);
    qDebug() << "resolveHostViaOpenDns: payload size" << payload.size();

    QNetworkReply *reply = amnApp->networkManager()->post(request, payload);
    if (!reply) {
        qWarning() << "Failed to create DoH request" << host;
        return {};
    }
    qDebug() << "resolveHostViaOpenDns: request sent" << host;

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    qDebug() << "resolveHostViaOpenDns: reply finished" << host;

    QByteArray dnsResponse;
    if (reply->error() == QNetworkReply::NoError) {
        dnsResponse = reply->readAll();
        qDebug() << "resolveHostViaOpenDns: received response size" << dnsResponse.size();
    } else {
        qWarning() << "DoH request failed" << host << reply->errorString();
    }

    reply->deleteLater();

    if (dnsResponse.isEmpty()) {
        return {};
    }

    const QString resolvedIp = parseDnsResponse(dnsResponse);
    qDebug() << "resolveHostViaOpenDns: parsed result" << resolvedIp;
    return resolvedIp;
}

QByteArray GatewayController::buildDnsQuery(const QString &host) const
{
    QByteArray query;
    QDataStream stream(&query, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    quint16 transactionId = QRandomGenerator::system()->generate();
    stream << transactionId;
    stream << static_cast<quint16>(0x0100); // standard query with recursion desired
    stream << static_cast<quint16>(1);      // QDCOUNT
    stream << static_cast<quint16>(0);      // ANCOUNT
    stream << static_cast<quint16>(0);      // NSCOUNT
    stream << static_cast<quint16>(0);      // ARCOUNT

    const QByteArray hostUtf8 = host.toUtf8();
    const QList<QByteArray> labels = hostUtf8.split('.');
    for (const QByteArray &label : labels) {
        stream << static_cast<quint8>(label.size());
        stream.writeRawData(label.constData(), label.size());
    }
    stream << static_cast<quint8>(0); // end of QNAME

    stream << static_cast<quint16>(1); // QTYPE A
    stream << static_cast<quint16>(1); // QCLASS IN

    return query;
}

QString GatewayController::parseDnsResponse(const QByteArray &response) const
{
    if (response.size() < 12) {
        qWarning() << "DNS response too short" << response.size();
        return {};
    }

    QDataStream stream(response);
    stream.setByteOrder(QDataStream::BigEndian);

    quint16 transactionId;
    quint16 flags;
    quint16 qdCount;
    quint16 anCount;
    quint16 nsCount;
    quint16 arCount;

    stream >> transactionId >> flags >> qdCount >> anCount >> nsCount >> arCount;

    if ((flags & 0x000F) != 0) {
        qWarning() << "DNS response contains error" << flags;
        return {};
    }

    int offset = 12;

    for (int i = 0; i < qdCount; ++i) {
        offset = skipDnsName(response, offset);
        if (offset < 0 || offset + 4 > response.size()) {
            qWarning() << "Invalid DNS question section";
            return {};
        }
        offset += 4;
    }

    const uchar *data = reinterpret_cast<const uchar *>(response.constData());
    for (int i = 0; i < anCount; ++i) {
        int nameOffset = skipDnsName(response, offset);
        if (nameOffset < 0 || nameOffset + 10 > response.size()) {
            qWarning() << "Invalid DNS answer section";
            return {};
        }
        offset = nameOffset;

        quint16 type = qFromBigEndian<quint16>(data + offset);
        quint16 dnsClass = qFromBigEndian<quint16>(data + offset + 2);
        quint32 ttl = qFromBigEndian<quint32>(data + offset + 4);
        Q_UNUSED(ttl);
    quint16 rdLength = qFromBigEndian<quint16>(data + offset + 8);
        offset += 10;

        if (offset + rdLength > response.size()) {
            qWarning() << "Invalid RDATA length" << rdLength;
            return {};
        }

        if (type == 1 && dnsClass == 1 && rdLength == 4) {
            const quint8 b1 = data[offset];
            const quint8 b2 = data[offset + 1];
            const quint8 b3 = data[offset + 2];
            const quint8 b4 = data[offset + 3];
            return QStringLiteral("%1.%2.%3.%4").arg(b1).arg(b2).arg(b3).arg(b4);
        }

        offset += rdLength;
    }

    return {};
}

int GatewayController::skipDnsName(const QByteArray &message, int offset) const
{
    while (offset < message.size()) {
        quint8 length = static_cast<quint8>(message.at(offset));
        if (length == 0) {
            return offset + 1;
        }
        if ((length & 0xC0) == 0xC0) {
            if (offset + 2 > message.size()) {
                return -1;
            }
            return offset + 2;
        }
        ++offset;
        offset += length;
        if (offset > message.size()) {
            return -1;
        }
    }
    return -1;
}
#endif