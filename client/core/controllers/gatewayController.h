#ifndef GATEWAYCONTROLLER_H
#define GATEWAYCONTROLLER_H

#include <QFuture>
#include <QJsonArray>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <memory>

#include "core/defs.h"
#include "core/transport/dns/dnsResolver.h"
#include "core/transport/igatewaytransport.h"

struct DnsTransportEntry
{
    amnezia::transport::dns::DnsProtocol type = amnezia::transport::dns::DnsProtocol::Udp;
    QString server;
    QString domain;
    quint16 port = 15353;
    QString dohPath = "/dns-query";

    bool isValid() const { return !server.isEmpty() && !domain.isEmpty(); }
};

enum class PrimaryTransport { Http, DnsUdp, DnsTcp, DnsDot, DnsDoh, DnsDoq };

struct TransportsConfig
{
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
    explicit GatewayController(const QString &gatewayEndpoint,
                               const bool isDevEnvironment,
                               const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled,
                               QObject *parent = nullptr);

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);

    static TransportsConfig buildTransportsConfig();
    void setTransportsConfig(const TransportsConfig &config);

private:
    struct EncryptedRequest
    {
        QByteArray body;
        QByteArray key;
        QByteArray iv;
        QByteArray salt;
        amnezia::ErrorCode errorCode = amnezia::ErrorCode::NoError;
    };

    EncryptedRequest encryptRequest(const QJsonObject &apiPayload);
    amnezia::transport::DecryptionResult decryptResponse(const QByteArray &encryptedResponseBody,
                                                         const QByteArray &key,
                                                         const QByteArray &iv,
                                                         const QByteArray &salt) const;

    std::shared_ptr<amnezia::transport::IGatewayTransport> currentTransport() const;
    static std::shared_ptr<amnezia::transport::IGatewayTransport> buildTransport(
            const TransportsConfig &config, int requestTimeoutMsecs, bool isDevEnvironment, bool isStrictKillSwitchEnabled);

    int m_requestTimeoutMsecs;
    QString m_gatewayEndpoint;
    bool m_isDevEnvironment = false;
    bool m_isStrictKillSwitchEnabled = false;

    mutable QMutex m_transportMutex;
    std::shared_ptr<amnezia::transport::IGatewayTransport> m_transport;
};

#endif // GATEWAYCONTROLLER_H
