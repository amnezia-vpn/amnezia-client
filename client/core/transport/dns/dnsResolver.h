#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <QString>

namespace amnezia::transport::dns
{

enum class DnsProtocol { Udp, Tcp, Tls, Https, Quic };

namespace DnsResolver
{
    QString resolve(const QString &hostname,
                    const QString &dnsServer,
                    DnsProtocol protocol,
                    quint16 port,
                    int timeoutMsecs = 3000,
                    const QString &dohEndpoint = QStringLiteral("/dns-query"));

    QString resolveOverUdp(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs = 3000);
    QString resolveOverTcp(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs = 3000);
    QString resolveOverTls(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs = 3000);
    QString resolveOverHttps(const QString &hostname, const QString &dnsServer, const QString &endpoint, int timeoutMsecs = 3000);
    QString resolveOverQuic(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs = 3000);
} // namespace DnsResolver

} // namespace amnezia::transport::dns

#endif // DNSRESOLVER_H
