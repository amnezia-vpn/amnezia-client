#ifndef DNSGATEWAYTRANSPORT_H
#define DNSGATEWAYTRANSPORT_H

#include <QString>
#include <atomic>

#include "dns/dnsResolver.h"
#include "igatewaytransport.h"

namespace amnezia::transport
{

class DnsGatewayTransport : public IGatewayTransport
{
public:
    DnsGatewayTransport(dns::DnsProtocol protocol,
                        const QString &dnsServer,
                        const QString &baseDomain,
                        quint16 port,
                        int timeoutMsecs,
                        bool isStrictKillSwitchEnabled,
                        const QString &dohEndpoint = QStringLiteral("/dns-query"));

    QString name() const override;

    amnezia::ErrorCode send(const QString &endpointTemplate,
                            const QByteArray &requestBody,
                            QByteArray &decryptedResponse,
                            const DecryptionHook &decryptionHook) override;

private:
    QString resolveServerOnce();
    void applyKillSwitchAllowlist(const QString &ip);

    dns::DnsProtocol m_protocol;
    QString m_dnsServer;
    QString m_baseDomain;
    quint16 m_port;
    int m_timeoutMsecs;
    bool m_isStrictKillSwitchEnabled;
    QString m_dohEndpoint;

    std::atomic_bool m_resolved{ false };
    QString m_resolvedServerIp;
};

} // namespace amnezia::transport

#endif // DNSGATEWAYTRANSPORT_H
