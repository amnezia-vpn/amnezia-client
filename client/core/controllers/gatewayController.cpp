#include "gatewayController.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <random>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QNetworkReply>
#include <QPromise>
#include <QUrl>
#include <QHostAddress>
#include <QHostInfo>
#include <QDebug>
#include <QThread>
#include <QtConcurrent>

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

    constexpr int httpStatusCodeNotFound = 404;
    constexpr int httpStatusCodeConflict = 409;

    constexpr int httpStatusCodeNotImplemented = 501;
}

// Parse TransportsConfig from JSON
TransportsConfig TransportsConfig::fromJson(const QJsonObject &json)
{
    TransportsConfig config;
    
    // Parse primary transport
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
    
    // Parse retry settings
    config.retryCount = json.value("retry_count").toInt(3);
    config.timeoutMs = json.value("timeout_ms").toInt(10000);
    
    // Parse HTTP config
    if (json.contains("http")) {
        QJsonObject httpObj = json["http"].toObject();
        config.httpEnabled = httpObj.value("enabled").toBool(true);
        config.httpEndpoint = httpObj.value("endpoint").toString();
    }
    
    // Parse DNS transports (each with its own server/domain)
    if (json.contains("dns_transports")) {
        QJsonArray transportsArray = json["dns_transports"].toArray();
        for (const auto &transportVal : transportsArray) {
            QJsonObject transportObj = transportVal.toObject();
            DnsTransportEntry entry;
            
            // Each transport has its own server and domain
            entry.server = transportObj.value("server").toString();
            entry.domain = transportObj.value("domain").toString();
            entry.port = static_cast<quint16>(transportObj.value("port").toInt(15353));
            entry.dohPath = transportObj.value("path").toString("/dns-query");
            
            QString typeStr = transportObj.value("type").toString().toLower();
            if (typeStr == "udp") {
                entry.type = NetworkUtilities::DnsTransport::Udp;
                if (entry.port == 15353 && !transportObj.contains("port")) {
                    entry.port = 15353;
                }
            } else if (typeStr == "tcp") {
                entry.type = NetworkUtilities::DnsTransport::Tcp;
                if (entry.port == 15353 && !transportObj.contains("port")) {
                    entry.port = 15353;
                }
            } else if (typeStr == "dot" || typeStr == "tls") {
                entry.type = NetworkUtilities::DnsTransport::Tls;
                if (!transportObj.contains("port")) {
                    entry.port = 8853;
                }
            } else if (typeStr == "doh" || typeStr == "https") {
                entry.type = NetworkUtilities::DnsTransport::Https;
                if (!transportObj.contains("port")) {
                    entry.port = 443;
                }
            } else if (typeStr == "doq" || typeStr == "quic") {
                entry.type = NetworkUtilities::DnsTransport::Quic;
                if (!transportObj.contains("port")) {
                    entry.port = 8853;
                }
            } else {
                continue;  // Skip unknown transport
            }
            
            if (entry.isValid()) {
                config.dnsTransports.append(entry);
            }
        }
    }
    
    return config;
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

void GatewayController::setDnsServer(const QString &dnsServer, const QString &baseDomain, 
                                      NetworkUtilities::DnsTransport transport, quint16 port, const QString &dohEndpoint)
{
    m_dnsServer = dnsServer;
    m_dnsBaseDomain = baseDomain;
    m_dnsTransport = transport;
    m_dnsPort = port;
    m_dohEndpoint = dohEndpoint;
    
    QString transportName;
    switch (transport) {
        case NetworkUtilities::DnsTransport::Udp: transportName = "UDP"; break;
        case NetworkUtilities::DnsTransport::Tcp: transportName = "TCP"; break;
        case NetworkUtilities::DnsTransport::Tls: transportName = "DoT"; break;
        case NetworkUtilities::DnsTransport::Https: transportName = "DoH"; break;
        case NetworkUtilities::DnsTransport::Quic: transportName = "DoQ"; break;
    }
    qDebug() << "[DNS Tunnel] Server:" << dnsServer << "BaseDomain:" << baseDomain 
             << "Transport:" << transportName << "Port:" << port;
}

void GatewayController::setTransportsConfig(const TransportsConfig &config)
{
    m_transportsConfig = config;
    
    // Update legacy fields for backward compatibility
    if (!config.httpEndpoint.isEmpty()) {
        m_gatewayEndpoint = config.httpEndpoint;
    }
    
    // Update timeout from config
    if (config.timeoutMs > 0) {
        m_requestTimeoutMsecs = config.timeoutMs;
    }
    
    qDebug() << "[Transport] Config set: HTTP enabled=" << config.httpEnabled 
             << "endpoint=" << config.httpEndpoint
             << "DNS transports=" << config.dnsTransports.size()
             << "primary=" << static_cast<int>(config.primary)
             << "retry=" << config.retryCount
             << "timeout=" << config.timeoutMs;
}

TransportsConfig GatewayController::buildTransportsConfig()
{
    TransportsConfig config;

    QString server(AGW_DNS_SERVER);
    QString domain(AGW_DNS_DOMAIN);

    if (server.isEmpty() || domain.isEmpty()) {
        qDebug() << "[Transport] DNS server/domain not configured, HTTP only";
        return config;
    }

    QString primaryStr = QString(AGW_DNS_PRIMARY).toLower();
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

    int retryCount = QString(AGW_DNS_RETRY_COUNT).toInt();
    config.retryCount = (retryCount > 0) ? retryCount : 3;

    int timeoutMs = QString(AGW_DNS_TIMEOUT_MS).toInt();
    config.timeoutMs = (timeoutMs > 0) ? timeoutMs : 10000;

    config.httpEnabled = true;

    auto addTransport = [&](NetworkUtilities::DnsTransport type, const char *portDefine, quint16 defaultPort,
                            const QString &dohPath = QString()) {
        DnsTransportEntry entry;
        entry.type = type;
        entry.server = server;
        entry.domain = domain;
        quint16 port = QString(portDefine).toUShort();
        entry.port = (port > 0) ? port : defaultPort;
        if (!dohPath.isEmpty()) entry.dohPath = dohPath;
        config.dnsTransports.append(entry);
    };

    addTransport(NetworkUtilities::DnsTransport::Udp, AGW_DNS_PORT_UDP, 5353);
    addTransport(NetworkUtilities::DnsTransport::Tcp, AGW_DNS_PORT_UDP, 5353);
    addTransport(NetworkUtilities::DnsTransport::Tls, AGW_DNS_PORT_DOT, 853);

    QString dohPath = QString(AGW_DNS_DOH_PATH);
    if (dohPath.isEmpty()) dohPath = "/dns-query";
    addTransport(NetworkUtilities::DnsTransport::Https, AGW_DNS_PORT_DOH, 443, dohPath);

    addTransport(NetworkUtilities::DnsTransport::Quic, AGW_DNS_PORT_DOQ, 8853);

    qDebug() << "[Transport] Built config from env: server=" << server << "domain=" << domain
             << "transports=" << config.dnsTransports.size() << "primary=" << static_cast<int>(config.primary);

    return config;
}

ErrorCode GatewayController::postParallel(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    if (!m_transportsConfig.isValid()) {
        qWarning() << "[Transport] Invalid config, falling back to HTTP only";
        return post(endpoint, apiPayload, responseBody);
    }
    
    // Prepare encrypted request once (skip DNS resolve for DNS tunneling)
    EncryptedRequestData encRequestData = prepareRequest(endpoint, apiPayload, true);
    if (encRequestData.errorCode != ErrorCode::NoError) {
        return encRequestData.errorCode;
    }
    
    // Extract endpoint name for DNS tunneling
    QString endpointName = endpoint;
    endpointName.remove("%1");
    if (endpointName.startsWith("v1/")) {
        endpointName = endpointName.mid(3);
    }
    if (endpointName.endsWith("/")) {
        endpointName.chop(1);
    }
    
    // Pre-resolve all DNS server hostnames to IPs (thread-safe: done before launching parallel threads)
    QMap<QString, QString> resolvedHosts;
    for (const auto &t : m_transportsConfig.dnsTransports) {
        if (!t.server.isEmpty() && !resolvedHosts.contains(t.server)) {
            QHostAddress addr(t.server);
            if (addr.isNull()) {
                QHostInfo info = QHostInfo::fromName(t.server);
                if (!info.addresses().isEmpty()) {
                    resolvedHosts[t.server] = info.addresses().first().toString();
                    qDebug() << "[Transport] Resolved" << t.server << "->" << resolvedHosts[t.server];
                } else {
                    resolvedHosts[t.server] = t.server;
                    qDebug() << "[Transport] Failed to resolve" << t.server;
                }
            } else {
                resolvedHosts[t.server] = t.server;
            }
        }
    }
    
    // Helper: find DNS transport by type
    auto findDnsTransport = [&](NetworkUtilities::DnsTransport type) -> const DnsTransportEntry* {
        for (const auto &t : m_transportsConfig.dnsTransports) {
            if (t.type == type && t.isValid()) return &t;
        }
        return nullptr;
    };
    
    // Helper: try HTTP transport
    auto tryHttp = [&]() -> ErrorCode {
        if (!m_transportsConfig.httpEnabled) return ErrorCode::AmneziaServiceConnectionFailed;
        
        qDebug() << "[Transport] PRIMARY: Trying HTTP";
        EncryptedRequestData httpRequestData = prepareRequest(endpoint, apiPayload, false);
        if (httpRequestData.errorCode != ErrorCode::NoError) return httpRequestData.errorCode;
        
        QNetworkAccessManager nam;
        QNetworkReply *reply = nam.post(httpRequestData.request, httpRequestData.requestBody);
        QEventLoop wait;
        QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
        wait.exec();
        
        QByteArray encryptedBody = reply->readAll();
        auto replyError = reply->error();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString errorStr = reply->errorString();
        reply->deleteLater();
        
        if (replyError != QNetworkReply::NoError || encryptedBody.isEmpty()) {
            qDebug() << "[Transport] PRIMARY HTTP failed:" << replyError << errorStr
                     << "status:" << httpStatus << "body size:" << encryptedBody.size();
            return ErrorCode::AmneziaServiceConnectionFailed;
        }
        
        try {
            QSimpleCrypto::QBlockCipher blockCipher;
            responseBody = blockCipher.decryptAesBlockCipher(encryptedBody, httpRequestData.key, httpRequestData.iv, "", httpRequestData.salt);
            qDebug() << "[Transport] PRIMARY HTTP succeeded";
            return ErrorCode::NoError;
        } catch (...) {
            return ErrorCode::ApiConfigDecryptionError;
        }
    };
    
    // Helper: try DNS transport
    auto tryDns = [&](const DnsTransportEntry &transport) -> ErrorCode {
        QString transportName;
        switch (transport.type) {
            case NetworkUtilities::DnsTransport::Udp: transportName = "UDP"; break;
            case NetworkUtilities::DnsTransport::Tcp: transportName = "TCP"; break;
            case NetworkUtilities::DnsTransport::Tls: transportName = "DoT"; break;
            case NetworkUtilities::DnsTransport::Https: transportName = "DoH"; break;
            case NetworkUtilities::DnsTransport::Quic: transportName = "DoQ"; break;
        }
        
        bool needsHostname = (transport.type == NetworkUtilities::DnsTransport::Https ||
                              transport.type == NetworkUtilities::DnsTransport::Tls);
        QString serverAddr = needsHostname ? transport.server : resolvedHosts.value(transport.server, transport.server);
        qDebug() << "[Transport] PRIMARY: Trying DNS" << transportName;
        QByteArray dnsResponse = NetworkUtilities::sendViaDnsTunnel(
            encRequestData.requestBody, endpointName, transport.domain,
            serverAddr, transport.type, transport.port, m_requestTimeoutMsecs, transport.dohPath);
        
        if (dnsResponse.isEmpty()) {
            qDebug() << "[Transport] PRIMARY DNS" << transportName << "failed";
            return ErrorCode::AmneziaServiceConnectionFailed;
        }
        
        try {
            QSimpleCrypto::QBlockCipher blockCipher;
            responseBody = blockCipher.decryptAesBlockCipher(dnsResponse, encRequestData.key, encRequestData.iv, "", encRequestData.salt);
            qDebug() << "[Transport] PRIMARY DNS" << transportName << "succeeded";
            return ErrorCode::NoError;
        } catch (...) {
            return ErrorCode::ApiConfigDecryptionError;
        }
    };
    
    // === STEP 1: Try PRIMARY transport first ===
    qDebug() << "[Transport] Trying primary transport:" << static_cast<int>(m_transportsConfig.primary);
    
    ErrorCode primaryResult = ErrorCode::AmneziaServiceConnectionFailed;
    switch (m_transportsConfig.primary) {
        case PrimaryTransport::Http:
            primaryResult = tryHttp();
            break;
        case PrimaryTransport::DnsUdp:
            if (auto t = findDnsTransport(NetworkUtilities::DnsTransport::Udp)) primaryResult = tryDns(*t);
            break;
        case PrimaryTransport::DnsTcp:
            if (auto t = findDnsTransport(NetworkUtilities::DnsTransport::Tcp)) primaryResult = tryDns(*t);
            break;
        case PrimaryTransport::DnsDot:
            if (auto t = findDnsTransport(NetworkUtilities::DnsTransport::Tls)) primaryResult = tryDns(*t);
            break;
        case PrimaryTransport::DnsDoh:
            if (auto t = findDnsTransport(NetworkUtilities::DnsTransport::Https)) primaryResult = tryDns(*t);
            break;
        case PrimaryTransport::DnsDoq:
            if (auto t = findDnsTransport(NetworkUtilities::DnsTransport::Quic)) primaryResult = tryDns(*t);
            break;
    }
    
    if (primaryResult == ErrorCode::NoError) {
        return ErrorCode::NoError;
    }
    
    // === STEP 2: Primary failed — launch ALL other transports in parallel ===
    qDebug() << "[Transport] Primary failed, launching parallel fallback";
    
    std::atomic<bool> gotSuccess{false};
    QByteArray successResult;
    QString successTransport;
    QMutex resultMutex;
    QList<QThread*> threads;
    
    auto launchThread = [&](std::function<void()> func) {
        QThread *thread = QThread::create(std::move(func));
        threads.append(thread);
        thread->start();
        QThread::msleep(10);
    };
    
    // HTTP (if not primary and enabled)
    if (m_transportsConfig.primary != PrimaryTransport::Http && m_transportsConfig.httpEnabled) {
        launchThread([&]() {
            if (gotSuccess.load()) return;
            
            qDebug() << "[Transport] FALLBACK: Trying HTTP";
            EncryptedRequestData httpRequestData = prepareRequest(endpoint, apiPayload, true);
            if (httpRequestData.errorCode != ErrorCode::NoError) return;
            
            QNetworkAccessManager nam;
            QNetworkReply *reply = nam.post(httpRequestData.request, httpRequestData.requestBody);
            QEventLoop wait;
            QObject::connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);
            wait.exec();
            
            if (gotSuccess.load()) { reply->deleteLater(); return; }
            
            QByteArray encryptedBody = reply->readAll();
            auto replyError = reply->error();
            reply->deleteLater();
            
            if (replyError != QNetworkReply::NoError || encryptedBody.isEmpty()) return;
            
            try {
                QSimpleCrypto::QBlockCipher blockCipher;
                QByteArray decrypted = blockCipher.decryptAesBlockCipher(encryptedBody, httpRequestData.key, httpRequestData.iv, "", httpRequestData.salt);
                if (!gotSuccess.exchange(true)) {
                    QMutexLocker lock(&resultMutex);
                    successResult = decrypted;
                    successTransport = "HTTP";
                }
            } catch (...) {}
        });
    }
    
    // DNS transports (skip the one that was primary)
    for (const auto &transport : m_transportsConfig.dnsTransports) {
        if (!transport.isValid()) continue;
        
        // Skip if this was primary
        bool wasPrimary = false;
        switch (m_transportsConfig.primary) {
            case PrimaryTransport::DnsUdp: wasPrimary = (transport.type == NetworkUtilities::DnsTransport::Udp); break;
            case PrimaryTransport::DnsTcp: wasPrimary = (transport.type == NetworkUtilities::DnsTransport::Tcp); break;
            case PrimaryTransport::DnsDot: wasPrimary = (transport.type == NetworkUtilities::DnsTransport::Tls); break;
            case PrimaryTransport::DnsDoh: wasPrimary = (transport.type == NetworkUtilities::DnsTransport::Https); break;
            case PrimaryTransport::DnsDoq: wasPrimary = (transport.type == NetworkUtilities::DnsTransport::Quic); break;
            default: break;
        }
        if (wasPrimary) continue;
        
        launchThread([&, transport]() {
            if (gotSuccess.load()) return;
            
            QString transportName;
            switch (transport.type) {
                case NetworkUtilities::DnsTransport::Udp: transportName = "UDP"; break;
                case NetworkUtilities::DnsTransport::Tcp: transportName = "TCP"; break;
                case NetworkUtilities::DnsTransport::Tls: transportName = "DoT"; break;
                case NetworkUtilities::DnsTransport::Https: transportName = "DoH"; break;
                case NetworkUtilities::DnsTransport::Quic: transportName = "DoQ"; break;
            }
            
            // TLS-based transports need original hostname for certificate validation
            bool needsHostname = (transport.type == NetworkUtilities::DnsTransport::Https ||
                                  transport.type == NetworkUtilities::DnsTransport::Tls);
            QString serverAddr = needsHostname ? transport.server : resolvedHosts.value(transport.server, transport.server);
            qDebug() << "[Transport] FALLBACK: Trying DNS" << transportName;
            QByteArray dnsResponse = NetworkUtilities::sendViaDnsTunnel(
                encRequestData.requestBody, endpointName, transport.domain,
                serverAddr, transport.type, transport.port, m_requestTimeoutMsecs, transport.dohPath);
            
            if (dnsResponse.isEmpty()) return;
            
            try {
                QSimpleCrypto::QBlockCipher blockCipher;
                QByteArray decrypted = blockCipher.decryptAesBlockCipher(dnsResponse, encRequestData.key, encRequestData.iv, "", encRequestData.salt);
                if (!gotSuccess.exchange(true)) {
                    QMutexLocker lock(&resultMutex);
                    successResult = decrypted;
                    successTransport = "DNS-" + transportName;
                }
            } catch (...) {}
        });
    }
    
    for (auto *t : threads) {
        t->wait();
        delete t;
    }
    
    if (gotSuccess.load()) {
        responseBody = successResult;
        qDebug() << "[Transport] FALLBACK success via" << successTransport;
        return ErrorCode::NoError;
    }
    
    qDebug() << "[Transport] All transports failed";
    return ErrorCode::AmneziaServiceConnectionFailed;
}

QString GatewayController::resolveGatewayHostname(const QString &hostname)
{
    if (m_dnsServer.isEmpty()) {
        return QString();
    }
    
    QString transportName;
    switch (m_dnsTransport) {
        case NetworkUtilities::DnsTransport::Udp: transportName = "UDP"; break;
        case NetworkUtilities::DnsTransport::Tcp: transportName = "TCP"; break;
        case NetworkUtilities::DnsTransport::Tls: transportName = "DoT"; break;
        case NetworkUtilities::DnsTransport::Https: transportName = "DoH"; break;
        case NetworkUtilities::DnsTransport::Quic: transportName = "DoQ"; break;
    }
    
    qDebug() << "[DNS] Resolving" << hostname << "via" << transportName << "server:" << m_dnsServer << "port:" << m_dnsPort;
    
    QString ip = NetworkUtilities::resolveDns(hostname, m_dnsServer, m_dnsTransport, m_dnsPort, 3000, m_dohEndpoint);
    
    if (!ip.isEmpty()) {
        qDebug() << "[DNS] Resolved:" << hostname << "->" << ip << "via" << transportName;
    } else {
        qDebug() << "[DNS] Resolution failed for:" << hostname << "via" << transportName;
    }
    
    return ip;
}

ErrorCode GatewayController::postViaDns(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    if (m_dnsServer.isEmpty() || m_dnsBaseDomain.isEmpty()) {
        qDebug() << "[DNS Tunnel] DNS server or base domain not set";
        return ErrorCode::AmneziaServiceConnectionFailed;
    }
    
    // Prepare encrypted request (skip DNS resolve - we send directly to m_dnsServer)
    EncryptedRequestData encRequestData = prepareRequest(endpoint, apiPayload, true);
    if (encRequestData.errorCode != ErrorCode::NoError) {
        return encRequestData.errorCode;
    }
    
    // Extract endpoint name from full path (e.g., "/v1/config" -> "config")
    QString endpointName = endpoint;
    endpointName.remove("%1");  // Remove placeholder
    if (endpointName.startsWith("v1/")) {
        endpointName = endpointName.mid(3);  // Remove "v1/"
    }
    if (endpointName.endsWith("/")) {
        endpointName.chop(1);
    }
    
    QString transportName;
    switch (m_dnsTransport) {
        case NetworkUtilities::DnsTransport::Udp: transportName = "UDP"; break;
        case NetworkUtilities::DnsTransport::Tcp: transportName = "TCP"; break;
        case NetworkUtilities::DnsTransport::Tls: transportName = "DoT"; break;
        case NetworkUtilities::DnsTransport::Https: transportName = "DoH"; break;
        case NetworkUtilities::DnsTransport::Quic: transportName = "DoQ"; break;
    }
    
    qDebug() << "[DNS Tunnel] Sending request to endpoint:" << endpointName 
             << "via" << transportName << "payload size:" << encRequestData.requestBody.size();
    
    // Send via DNS tunnel
    QByteArray dnsResponse = NetworkUtilities::sendViaDnsTunnel(
        encRequestData.requestBody, endpointName, m_dnsBaseDomain,
        m_dnsServer, m_dnsTransport, m_dnsPort, m_requestTimeoutMsecs, m_dohEndpoint);
    
    if (dnsResponse.isEmpty()) {
        qDebug() << "[DNS Tunnel] Empty response";
        return ErrorCode::AmneziaServiceConnectionFailed;
    }
    
    qDebug() << "[DNS Tunnel] Received response:" << dnsResponse.size() << "bytes";
    
    // Decrypt response
    try {
        QSimpleCrypto::QBlockCipher blockCipher;
        responseBody = blockCipher.decryptAesBlockCipher(dnsResponse, encRequestData.key, encRequestData.iv, "", encRequestData.salt);
        qDebug() << "[DNS Tunnel] Decrypted response:" << responseBody.left(200);
        return ErrorCode::NoError;
    } catch (...) {
        qDebug() << "[DNS Tunnel] Failed to decrypt response, returning raw data";
        responseBody = dnsResponse;
        return ErrorCode::NoError;
    }
}

GatewayController::EncryptedRequestData GatewayController::prepareRequest(const QString &endpoint, const QJsonObject &apiPayload, bool skipDnsResolve)
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
    
    // DNS резолв через TCP/UDP (пропускаем для DNS tunneling — там запрос идёт напрямую к DNS серверу)
    QString finalGatewayEndpoint = m_proxyUrl.isEmpty() ? m_gatewayEndpoint : m_proxyUrl;
    QUrl gatewayUrl(finalGatewayEndpoint);
    QString hostname = gatewayUrl.host();
    
    // Проверяем, нужно ли резолвить (если это не IP адрес и не localhost)
    if (!skipDnsResolve &&
        !hostname.isEmpty() && 
        hostname != "localhost" &&
        !NetworkUtilities::checkIPv4Format(hostname) && 
        QHostAddress(hostname).isNull()) {
        
        QString resolvedIp = resolveGatewayHostname(hostname);
        if (!resolvedIp.isEmpty()) {
            gatewayUrl.setHost(resolvedIp);
            finalGatewayEndpoint = gatewayUrl.toString();
            qDebug() << "DNS resolved:" << hostname << "->" << resolvedIp;
        } else {
            // Fallback: используем оригинальный hostname
            qWarning() << "DNS resolution failed for:" << hostname << ", using original hostname";
        }
    }
    
    encRequestData.request.setUrl(endpoint.arg(finalGatewayEndpoint));

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
    keyPayload[configKey::aesKey] = QString(encRequestData.key.toBase64());
    keyPayload[configKey::aesIv] = QString(encRequestData.iv.toBase64());
    keyPayload[configKey::aesSalt] = QString(encRequestData.salt.toBase64());

    QByteArray encryptedKeyPayload;
    QByteArray encryptedApiPayload;
    try {
        QSimpleCrypto::QRsa rsa;

        EVP_PKEY *publicKey = nullptr;
        try {
            QByteArray rsaKey = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
            rsaKey.replace("\\n", "\n");
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
    requestBody[configKey::keyPayload] = QString(encryptedKeyPayload.toBase64());
    requestBody[configKey::apiPayload] = QString(encryptedApiPayload.toBase64());

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

    connect(reply, &QNetworkReply::finished, reply, [promise, sslErrors, encRequestData, endpoint, apiPayload, reply, this]() mutable {
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

        if (sslErrors->isEmpty() && shouldBypassProxy(replyError, decryptionResult.decryptedBody, decryptionResult.isDecryptionSuccessful)) {
            auto serviceType = apiPayload.value(apiDefs::key::serviceType).toString("");
            auto userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString("");

            QStringList baseUrls;
            if (m_isDevEnvironment) {
                baseUrls = QString(DEV_S3_ENDPOINT).split(", ");
            } else {
                baseUrls = QString(PROD_S3_ENDPOINT).split(", ");
            }

            QStringList proxyStorageUrls;
            if (!serviceType.isEmpty()) {
                for (const auto &baseUrl : baseUrls) {
                    QByteArray path = ("endpoints-" + serviceType + "-" + userCountryCode).toUtf8();
                    proxyStorageUrls.push_back(baseUrl + path.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
                                               + ".json");
                }
            }
            for (const auto &baseUrl : baseUrls)
                proxyStorageUrls.push_back(baseUrl + "endpoints.json");

            getProxyUrlsAsync(proxyStorageUrls, 0, [this, encRequestData, endpoint, processResponse](const QStringList &proxyUrls) {
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
    request.setTransferTimeout(m_requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QEventLoop wait;
    QList<QSslError> sslErrors;
    QNetworkReply *reply;

    QStringList baseUrls;
    if (m_isDevEnvironment) {
        baseUrls = QString(DEV_S3_ENDPOINT).split(", ");
    } else {
        baseUrls = QString(PROD_S3_ENDPOINT).split(", ");
    }

    QByteArray key = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;

    QStringList proxyStorageUrls;
    if (!serviceType.isEmpty()) {
        for (const auto &baseUrl : baseUrls) {
            QByteArray path = ("endpoints-" + serviceType + "-" + userCountryCode).toUtf8();
            proxyStorageUrls.push_back(baseUrl + path.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals) + ".json");
        }
    }
    for (const auto &baseUrl : baseUrls) {
        proxyStorageUrls.push_back(baseUrl + "endpoints.json");
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
    const QByteArray &responseBody = decryptedResponseBody;

    int httpStatus = -1;
    if (isDecryptionSuccessful) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseBody);
        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            httpStatus = jsonObj.value("http_status").toInt(-1);
        }
    } else {
        qDebug() << "failed to decrypt the data";
        return true;
    }

    if (replyError == QNetworkReply::NetworkError::OperationCanceledError || replyError == QNetworkReply::NetworkError::TimeoutError) {
        qDebug() << "timeout occurred";
        qDebug() << replyError;
        return true;
    } else if (responseBody.contains("html")) {
        qDebug() << "the response contains an html tag";
        return true;
    } else if (httpStatus == httpStatusCodeNotFound) {
        if (responseBody.contains(errorResponsePattern1) || responseBody.contains(errorResponsePattern2)
            || responseBody.contains(errorResponsePattern3)) {
            return false;
        } else {
            qDebug() << replyError;
            return true;
        }
    } else if (httpStatus == httpStatusCodeNotImplemented) {
        if (responseBody.contains(updateRequestResponsePattern)) {
            return false;
        } else {
            qDebug() << replyError;
            return true;
        }
    } else if (httpStatus == httpStatusCodeConflict) {
        return false;
    } else if (replyError != QNetworkReply::NetworkError::NoError) {
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
                                          std::function<void(const QStringList &)> onComplete)
{
    if (currentProxyStorageIndex >= proxyStorageUrls.size()) {
        onComplete({});
        return;
    }

    QNetworkRequest request;
    request.setTransferTimeout(m_requestTimeoutMsecs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(proxyStorageUrls[currentProxyStorageIndex]);

    QNetworkReply *reply = amnApp->networkManager()->get(request);

    // connect(reply, &QNetworkReply::sslErrors, this, [state](const QList<QSslError> &e) { *(state->sslErrors) = e; });

    connect(reply, &QNetworkReply::finished, this, [this, proxyStorageUrls, currentProxyStorageIndex, onComplete, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray encrypted = reply->readAll();
            reply->deleteLater();

            QByteArray responseBody;
            try {
                QByteArray key = m_isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
                if (!m_isDevEnvironment) {
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
                QMetaObject::invokeMethod(
                        this, [=]() { getProxyUrlsAsync(proxyStorageUrls, currentProxyStorageIndex + 1, onComplete); }, Qt::QueuedConnection);
                return;
            }

            QJsonArray endpointsArray = QJsonDocument::fromJson(responseBody).array();
            QStringList endpoints;
            for (const QJsonValue &endpoint : endpointsArray)
                endpoints.push_back(endpoint.toString());

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
        QMetaObject::invokeMethod(
                this, [=]() { getProxyUrlsAsync(proxyStorageUrls, currentProxyStorageIndex + 1, onComplete); }, Qt::QueuedConnection);
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
