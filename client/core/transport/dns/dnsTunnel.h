#ifndef DNSTUNNEL_H
#define DNSTUNNEL_H

#include <QByteArray>
#include <QString>

#include "dnsResolver.h"

namespace amnezia::transport::dns::DnsTunnel
{

QByteArray send(const QByteArray &payload,
                const QString &endpointName,
                const QString &baseDomain,
                const QString &dnsServer,
                DnsProtocol protocol,
                quint16 port,
                int timeoutMsecs = 30000,
                const QString &dohEndpoint = QStringLiteral("/dns-query"));

QByteArray sendOverUdp(const QByteArray &payload, const QString &queryName,
                       const QString &dnsServer, quint16 port, int timeoutMsecs);
QByteArray sendOverTcp(const QByteArray &payload, const QString &queryName,
                       const QString &dnsServer, quint16 port, int timeoutMsecs);
QByteArray sendOverTls(const QByteArray &payload, const QString &queryName,
                       const QString &dnsServer, quint16 port, int timeoutMsecs);
QByteArray sendOverHttps(const QByteArray &payload, const QString &queryName,
                         const QString &dnsServer, quint16 port, const QString &endpoint, int timeoutMsecs);

QByteArray sendOverUdpChunked(const QByteArray &payload, const QString &queryName,
                              const QString &dnsServer, quint16 port, int timeoutMsecs);

} // namespace amnezia::transport::dns::DnsTunnel

#endif // DNSTUNNEL_H
