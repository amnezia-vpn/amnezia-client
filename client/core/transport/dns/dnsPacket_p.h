#ifndef DNSPACKET_P_H
#define DNSPACKET_P_H

#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include <QtEndian>

namespace amnezia::transport::dns::detail
{

constexpr quint16 DNS_PORT = 53;
constexpr quint16 DNS_TYPE_A = 1;
constexpr quint16 DNS_CLASS_IN = 1;

#pragma pack(push, 1)
struct DnsHeader
{
    quint16 id;
    quint16 flags;
    quint16 qdcount;
    quint16 ancount;
    quint16 nscount;
    quint16 arcount;
};
#pragma pack(pop)

QHostAddress resolveHostAddress(const QString &host);

QByteArray encodeDnsName(const QString &hostname);

QByteArray buildDnsQuery(const QString &hostname, quint16 transactionId);

QString parseDnsResponse(const QByteArray &response, bool isTcp);

} // namespace amnezia::transport::dns::detail

#endif // DNSPACKET_P_H
