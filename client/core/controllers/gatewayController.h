#ifndef GATEWAYCONTROLLER_H
#define GATEWAYCONTROLLER_H

#include <QFuture>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QObject>
#include <QPair>
#include <QPromise>
#include <QSharedPointer>

#include "core/defs.h"
#include "core/networkUtilities.h"

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

// NEW: Configuration for a single DNS transport (each can have its own server/domain)
struct DnsTransportEntry {
    NetworkUtilities::DnsTransport type = NetworkUtilities::DnsTransport::Udp;
    QString server;   // DNS server IP
    QString domain;   // Base domain for tunneling
    quint16 port = 15353;
    QString dohPath = "/dns-query";
    
    bool isValid() const { return !server.isEmpty() && !domain.isEmpty(); }
};

// NEW: Primary transport type
enum class PrimaryTransport { Http, DnsUdp, DnsTcp, DnsDot, DnsDoh, DnsDoq };

// NEW: Full transports configuration
struct TransportsConfig {
    PrimaryTransport primary = PrimaryTransport::Http;
    bool httpEnabled = true;
    QString httpEndpoint;
    QList<DnsTransportEntry> dnsTransports;
    int retryCount = 3;
    int timeoutMs = 10000;
    
    bool isValid() const { return httpEnabled || !dnsTransports.isEmpty(); }
    static TransportsConfig fromJson(const QJsonObject &json);
};

class GatewayController : public QObject
{
    Q_OBJECT

public:
    explicit GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, QObject *parent = nullptr);

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);
    
    // DNS transport settings
    void setDnsServer(const QString &dnsServer, const QString &baseDomain, NetworkUtilities::DnsTransport transport, 
                      quint16 port, const QString &dohEndpoint = "/dns-query");
    
    // DNS tunneling - send request via DNS transport
    amnezia::ErrorCode postViaDns(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    
    static TransportsConfig buildTransportsConfig();
    void setTransportsConfig(const TransportsConfig &config);
    
    // NEW: Parallel request via all configured transports (primary first, then others)
    amnezia::ErrorCode postParallel(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);

private:
    struct EncryptedRequestData
    {
        QNetworkRequest request;
        QByteArray requestBody;
        QByteArray key;
        QByteArray iv;
        QByteArray salt;
        amnezia::ErrorCode errorCode;
    };

    struct DecryptionResult
    {
        QByteArray decryptedBody;
        bool isDecryptionSuccessful;
    };

    EncryptedRequestData prepareRequest(const QString &endpoint, const QJsonObject &apiPayload, bool skipDnsResolve = false);
    DecryptionResult tryDecryptResponseBody(const QByteArray &encryptedResponseBody, QNetworkReply::NetworkError replyError,
                                            const QByteArray &key, const QByteArray &iv, const QByteArray &salt);
    QString resolveGatewayHostname(const QString &hostname);
    
    QStringList getProxyUrls(const QString &serviceType, const QString &userCountryCode);
    bool shouldBypassProxy(const QNetworkReply::NetworkError &replyError, const QByteArray &decryptedResponseBody, bool isDecryptionSuccessful);
    void bypassProxy(const QString &endpoint, const QString &serviceType, const QString &userCountryCode,
                     std::function<QNetworkReply *(const QString &url)> requestFunction,
                     std::function<bool(QNetworkReply *reply, const QList<QSslError> &sslErrors)> replyProcessingFunction);

    void getProxyUrlsAsync(const QStringList proxyStorageUrls, const int currentProxyStorageIndex,
                           std::function<void(const QStringList &)> onComplete);
    void getProxyUrlAsync(const QStringList proxyUrls, const int currentProxyIndex, std::function<void(const QString &)> onComplete);
    void bypassProxyAsync(
            const QString &endpoint, const QString &proxyUrl, EncryptedRequestData encRequestData,
            std::function<void(const QByteArray &, bool, const QList<QSslError> &, QNetworkReply::NetworkError, const QString &, int)> onComplete);

    int m_requestTimeoutMsecs;
    QString m_gatewayEndpoint;
    bool m_isDevEnvironment = false;
    bool m_isStrictKillSwitchEnabled = false;
    
    // DNS transport settings (for individual transport)
    QString m_dnsServer;
    QString m_dnsBaseDomain;
    NetworkUtilities::DnsTransport m_dnsTransport = NetworkUtilities::DnsTransport::Udp;
    quint16 m_dnsPort = 15353;
    QString m_dohEndpoint = "/dns-query";
    
    // NEW: Full transports configuration
    TransportsConfig m_transportsConfig;

    inline static QString m_proxyUrl;
};

#endif // GATEWAYCONTROLLER_H
