#include "gatewayController.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QSharedPointer>
#include <QThread>
#include <QtConcurrent>

#include "QBlockCipher.h"
#include "QRsa.h"

#include "amnezia_application.h"
#include "core/transport/dnsGatewayTransport.h"
#include "core/transport/httpGatewayTransport.h"
#include "utilities.h"

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

    amnezia::transport::dns::DnsProtocol dnsProtocolFromPrimary(PrimaryTransport p)
    {
        switch (p) {
        case PrimaryTransport::DnsUdp: return amnezia::transport::dns::DnsProtocol::Udp;
        case PrimaryTransport::DnsTcp: return amnezia::transport::dns::DnsProtocol::Tcp;
        case PrimaryTransport::DnsDot: return amnezia::transport::dns::DnsProtocol::Tls;
        case PrimaryTransport::DnsDoh: return amnezia::transport::dns::DnsProtocol::Https;
        case PrimaryTransport::DnsDoq: return amnezia::transport::dns::DnsProtocol::Quic;
        default: return amnezia::transport::dns::DnsProtocol::Udp;
        }
    }
} // namespace

TransportsConfig TransportsConfig::fromJson(const QJsonObject &json)
{
    using amnezia::transport::dns::DnsProtocol;

    TransportsConfig config;

    QString primaryStr = json.value("primary").toString("http").toLower();
    if (primaryStr == "http") {
        config.primary = PrimaryTransport::Http;
    } else if (primaryStr == "dns_udp" || primaryStr == "udp") {
        config.primary = PrimaryTransport::DnsUdp;
    } else if (primaryStr == "dns_tcp" || primaryStr == "tcp") {
        config.primary = PrimaryTransport::DnsTcp;
    } else if (primaryStr == "dns_dot" || primaryStr == "dot") {
        config.primary = PrimaryTransport::DnsDot;
    } else if (primaryStr == "dns_doh" || primaryStr == "doh") {
        config.primary = PrimaryTransport::DnsDoh;
    } else if (primaryStr == "dns_doq" || primaryStr == "doq") {
        config.primary = PrimaryTransport::DnsDoq;
    }

    config.retryCount = json.value("retry_count").toInt(3);
    config.timeoutMs = json.value("timeout_ms").toInt(10000);

    if (json.contains("http")) {
        QJsonObject httpObj = json["http"].toObject();
        config.httpEnabled = httpObj.value("enabled").toBool(true);
        config.httpEndpoint = httpObj.value("endpoint").toString();
    }

    if (json.contains("dns_transports")) {
        QJsonArray transportsArray = json["dns_transports"].toArray();
        for (const auto &transportVal : transportsArray) {
            QJsonObject transportObj = transportVal.toObject();
            DnsTransportEntry entry;

            entry.server = transportObj.value("server").toString();
            entry.domain = transportObj.value("domain").toString();
            entry.port = static_cast<quint16>(transportObj.value("port").toInt(15353));
            entry.dohPath = transportObj.value("path").toString("/dns-query");

            QString typeStr = transportObj.value("type").toString().toLower();
            if (typeStr == "udp") {
                entry.type = DnsProtocol::Udp;
            } else if (typeStr == "tcp") {
                entry.type = DnsProtocol::Tcp;
            } else if (typeStr == "dot" || typeStr == "tls") {
                entry.type = DnsProtocol::Tls;
                if (!transportObj.contains("port")) entry.port = 8853;
            } else if (typeStr == "doh" || typeStr == "https") {
                entry.type = DnsProtocol::Https;
                if (!transportObj.contains("port")) entry.port = 443;
            } else if (typeStr == "doq" || typeStr == "quic") {
                entry.type = DnsProtocol::Quic;
                if (!transportObj.contains("port")) entry.port = 8853;
            } else {
                continue;
            }

            if (entry.isValid()) {
                config.dnsTransports.append(entry);
            }
        }
    }

    return config;
}

GatewayController::GatewayController(const QString &gatewayEndpoint,
                                     const bool isDevEnvironment,
                                     const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled,
                                     QObject *parent)
    : QObject(parent),
      m_requestTimeoutMsecs(requestTimeoutMsecs),
      m_gatewayEndpoint(gatewayEndpoint),
      m_isDevEnvironment(isDevEnvironment),
      m_isStrictKillSwitchEnabled(isStrictKillSwitchEnabled)
{
    auto httpTransport = std::make_shared<amnezia::transport::HttpGatewayTransport>(
            m_gatewayEndpoint, m_isDevEnvironment, m_requestTimeoutMsecs, m_isStrictKillSwitchEnabled);
    {
        QMutexLocker lock(&m_transportMutex);
        m_transport = std::move(httpTransport);
    }
}

std::shared_ptr<amnezia::transport::IGatewayTransport> GatewayController::buildTransport(
        const TransportsConfig &config, int requestTimeoutMsecs, bool isDevEnvironment, bool isStrictKillSwitchEnabled)
{
    using namespace amnezia::transport;

    auto makeHttp = [&](const QString &httpEndpoint) {
        return std::make_shared<HttpGatewayTransport>(
                httpEndpoint, isDevEnvironment, requestTimeoutMsecs, isStrictKillSwitchEnabled);
    };

    if (config.primary == PrimaryTransport::Http) {
        return makeHttp(config.httpEndpoint);
    }

    const auto wantedProtocol = dnsProtocolFromPrimary(config.primary);
    for (const auto &entry : config.dnsTransports) {
        if (entry.type == wantedProtocol && entry.isValid()) {
            return std::make_shared<DnsGatewayTransport>(
                    entry.type, entry.server, entry.domain, entry.port,
                    requestTimeoutMsecs, isStrictKillSwitchEnabled, entry.dohPath);
        }
    }

    return makeHttp(config.httpEndpoint);
}

void GatewayController::setTransportsConfig(const TransportsConfig &config)
{
    if (config.timeoutMs > 0) {
        m_requestTimeoutMsecs = config.timeoutMs;
    }
    if (!config.httpEndpoint.isEmpty()) {
        m_gatewayEndpoint = config.httpEndpoint;
    }

    TransportsConfig effective = config;
    if (effective.httpEndpoint.isEmpty()) {
        effective.httpEndpoint = m_gatewayEndpoint;
    }

    auto newTransport = buildTransport(effective, m_requestTimeoutMsecs, m_isDevEnvironment, m_isStrictKillSwitchEnabled);
    QString activeName;
    {
        QMutexLocker lock(&m_transportMutex);
        m_transport = std::move(newTransport);
        activeName = m_transport ? m_transport->name() : QStringLiteral("none");
    }

    qDebug() << "[Transport] Active transport set to" << activeName;
}

TransportsConfig GatewayController::buildTransportsConfig()
{
    using amnezia::transport::dns::DnsProtocol;

    TransportsConfig config;

    QString server = QString(AGW_DNS_SERVER).trimmed();
    QString domain = QString(AGW_DNS_DOMAIN).trimmed();

    if (server.isEmpty() || domain.isEmpty()) {
        qDebug() << "[Transport] DNS server/domain not configured, HTTP only";
        return config;
    }

    QString primaryStr = QString(AGW_DNS_PRIMARY).trimmed().toLower();
    if (primaryStr == "udp" || primaryStr == "dns_udp") {
        config.primary = PrimaryTransport::DnsUdp;
    } else if (primaryStr == "tcp" || primaryStr == "dns_tcp") {
        config.primary = PrimaryTransport::DnsTcp;
    } else if (primaryStr == "dot" || primaryStr == "dns_dot") {
        config.primary = PrimaryTransport::DnsDot;
    } else if (primaryStr == "doh" || primaryStr == "dns_doh") {
        config.primary = PrimaryTransport::DnsDoh;
    } else if (primaryStr == "doq" || primaryStr == "dns_doq") {
        config.primary = PrimaryTransport::DnsDoq;
    } else {
        config.primary = PrimaryTransport::Http;
    }

    int retryCount = QString(AGW_DNS_RETRY_COUNT).trimmed().toInt();
    config.retryCount = (retryCount > 0) ? retryCount : 3;

    int timeoutMs = QString(AGW_DNS_TIMEOUT_MS).trimmed().toInt();
    config.timeoutMs = (timeoutMs > 0) ? timeoutMs : 10000;

    config.httpEnabled = true;

    auto addTransport = [&](DnsProtocol type, const char *portDefine, quint16 defaultPort,
                            const QString &dohPath = QString()) {
        DnsTransportEntry entry;
        entry.type = type;
        entry.server = server;
        entry.domain = domain;
        quint16 port = QString(portDefine).trimmed().toUShort();
        entry.port = (port > 0) ? port : defaultPort;
        if (!dohPath.isEmpty()) entry.dohPath = dohPath;
        config.dnsTransports.append(entry);
    };

    addTransport(DnsProtocol::Udp, AGW_DNS_PORT_UDP, 5353);
    addTransport(DnsProtocol::Tcp, AGW_DNS_PORT_UDP, 5353);
    addTransport(DnsProtocol::Tls, AGW_DNS_PORT_DOT, 853);

    QString dohPath = QString(AGW_DNS_DOH_PATH).trimmed();
    if (dohPath.isEmpty()) dohPath = "/dns-query";
    addTransport(DnsProtocol::Https, AGW_DNS_PORT_DOH, 443, dohPath);

    addTransport(DnsProtocol::Quic, AGW_DNS_PORT_DOQ, 8853);

    qDebug() << "[Transport] Built config from env: server=" << server << "domain=" << domain
             << "transports=" << config.dnsTransports.size() << "primary=" << static_cast<int>(config.primary);

    return config;
}

GatewayController::EncryptedRequest GatewayController::encryptRequest(const QJsonObject &apiPayload)
{
    EncryptedRequest result;
    result.errorCode = amnezia::ErrorCode::NoError;

    QSimpleCrypto::QBlockCipher blockCipher;
    result.key = blockCipher.generatePrivateSalt(32);
    result.iv = blockCipher.generatePrivateSalt(16);
    result.salt = blockCipher.generatePrivateSalt(8);

    QJsonObject keyPayload;
    keyPayload[configKey::aesKey] = QString(result.key.toBase64());
    keyPayload[configKey::aesIv] = QString(result.iv.toBase64());
    keyPayload[configKey::aesSalt] = QString(result.salt.toBase64());

    QByteArray encryptedKeyPayload;
    QByteArray encryptedApiPayload;
    try {
        QSimpleCrypto::QRsa rsa;
        EVP_PKEY *publicKey = nullptr;
        try {
            QByteArray rsaKey = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
            rsaKey = rsaKey.trimmed();
            rsaKey.replace("\\n", "\n");
            publicKey = rsa.getPublicKeyFromByteArray(rsaKey);
        } catch (...) {
            Utils::logException();
            qCritical() << "error loading public key from environment variables";
            result.errorCode = amnezia::ErrorCode::ApiMissingAgwPublicKey;
            return result;
        }

        encryptedKeyPayload = rsa.encrypt(QJsonDocument(keyPayload).toJson(QJsonDocument::Compact),
                                          publicKey, RSA_PKCS1_PADDING);
        EVP_PKEY_free(publicKey);

        encryptedApiPayload = blockCipher.encryptAesBlockCipher(QJsonDocument(apiPayload).toJson(QJsonDocument::Compact),
                                                                result.key, result.iv, "", result.salt);
    } catch (...) {
        Utils::logException();
        qCritical() << "error when encrypting the request body";
        result.errorCode = amnezia::ErrorCode::ApiConfigDecryptionError;
        return result;
    }

    QJsonObject requestBody;
    requestBody[configKey::keyPayload] = QString(encryptedKeyPayload.toBase64());
    requestBody[configKey::apiPayload] = QString(encryptedApiPayload.toBase64());

    result.body = QJsonDocument(requestBody).toJson(QJsonDocument::Compact);
    return result;
}

amnezia::transport::DecryptionResult GatewayController::decryptResponse(const QByteArray &encryptedResponseBody,
                                                                         const QByteArray &key,
                                                                         const QByteArray &iv,
                                                                         const QByteArray &salt) const
{
    amnezia::transport::DecryptionResult result;
    result.decrypted = encryptedResponseBody;
    result.isOk = false;

    if (encryptedResponseBody.isEmpty()) {
        return result;
    }

    try {
        QSimpleCrypto::QBlockCipher blockCipher;
        result.decrypted = blockCipher.decryptAesBlockCipher(encryptedResponseBody, key, iv, "", salt);
        result.isOk = true;
    } catch (...) {
        result.decrypted = encryptedResponseBody;
        result.isOk = false;
    }

    return result;
}

std::shared_ptr<amnezia::transport::IGatewayTransport> GatewayController::currentTransport() const
{
    QMutexLocker lock(&m_transportMutex);
    return m_transport;
}

amnezia::ErrorCode GatewayController::post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    EncryptedRequest enc = encryptRequest(apiPayload);
    if (enc.errorCode != amnezia::ErrorCode::NoError) {
        return enc.errorCode;
    }

    auto transport = currentTransport();
    if (!transport) {
        return amnezia::ErrorCode::AmneziaServiceConnectionFailed;
    }

    auto decryptionHook = [this, key = enc.key, iv = enc.iv, salt = enc.salt](const QByteArray &encrypted) {
        return decryptResponse(encrypted, key, iv, salt);
    };

    return transport->send(endpoint, enc.body, responseBody, decryptionHook);
}

QFuture<QPair<amnezia::ErrorCode, QByteArray>> GatewayController::postAsync(const QString &endpoint, const QJsonObject apiPayload)
{
    return QtConcurrent::run([this, endpoint, apiPayload]() {
        QByteArray responseBody;
        amnezia::ErrorCode errorCode = post(endpoint, apiPayload, responseBody);
        return qMakePair(errorCode, responseBody);
    });
}
