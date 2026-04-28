#include "dnsPacket_p.h"

#include <QHostInfo>
#include <cstring>

namespace amnezia::transport::dns::detail
{

QHostAddress resolveHostAddress(const QString &host)
{
    QHostAddress addr(host);
    if (!addr.isNull()) return addr;
    QHostInfo info = QHostInfo::fromName(host);
    if (!info.addresses().isEmpty()) return info.addresses().first();
    return QHostAddress();
}

QByteArray encodeDnsName(const QString &hostname)
{
    QByteArray result;
    const QStringList parts = hostname.split('.');

    for (const QString &part : parts) {
        if (part.length() > 63) {
            return QByteArray();
        }
        result.append(static_cast<char>(part.length()));
        result.append(part.toUtf8());
    }
    result.append(static_cast<char>(0));
    return result;
}

QByteArray buildDnsQuery(const QString &hostname, quint16 transactionId)
{
    QByteArray packet;

    DnsHeader header;
    header.id = qToBigEndian(transactionId);
    header.flags = qToBigEndian<quint16>(0x0100);
    header.qdcount = qToBigEndian<quint16>(1);
    header.ancount = 0;
    header.nscount = 0;
    header.arcount = 0;

    packet.append(reinterpret_cast<const char *>(&header), sizeof(DnsHeader));

    const QByteArray qname = encodeDnsName(hostname);
    if (qname.isEmpty()) {
        return QByteArray();
    }
    packet.append(qname);

    quint16 qtype = qToBigEndian<quint16>(DNS_TYPE_A);
    packet.append(reinterpret_cast<const char *>(&qtype), sizeof(quint16));

    quint16 qclass = qToBigEndian<quint16>(DNS_CLASS_IN);
    packet.append(reinterpret_cast<const char *>(&qclass), sizeof(quint16));

    return packet;
}

QString parseDnsResponse(const QByteArray &response, bool isTcp)
{
    if (response.size() < static_cast<int>(sizeof(DnsHeader))) {
        return QString();
    }

    int offset = isTcp ? 2 : 0;
    if (response.size() < offset + static_cast<int>(sizeof(DnsHeader))) {
        return QString();
    }

    DnsHeader header;
    std::memcpy(&header, response.constData() + offset, sizeof(DnsHeader));
    offset += sizeof(DnsHeader);

    const quint16 flags = qFromBigEndian(header.flags);
    const quint16 ancount = qFromBigEndian(header.ancount);

    if ((flags & 0x8000) == 0 || (flags & 0x000F) != 0) {
        return QString();
    }

    if (ancount == 0) {
        return QString();
    }

    while (offset < response.size() && response.at(offset) != 0) {
        const quint8 length = static_cast<quint8>(response.at(offset));
        if (length > 63) {
            return QString();
        }
        offset += length + 1;
    }
    if (offset >= response.size()) {
        return QString();
    }
    offset++;

    offset += 4;

    for (int i = 0; i < ancount && offset < response.size(); ++i) {
        if (offset >= response.size()) {
            break;
        }

        const quint8 nameByte = static_cast<quint8>(response.at(offset));
        if ((nameByte & 0xC0) == 0xC0) {
            offset += 2;
        } else {
            while (offset < response.size() && response.at(offset) != 0) {
                const quint8 length = static_cast<quint8>(response.at(offset));
                if (length > 63) {
                    return QString();
                }
                offset += length + 1;
            }
            offset++;
        }

        if (offset + 10 > response.size()) {
            break;
        }

        const quint16 type =
                qFromBigEndian<quint16>(*reinterpret_cast<const quint16 *>(response.constData() + offset));
        offset += 2;
        offset += 2;
        offset += 4;

        const quint16 rdlength =
                qFromBigEndian<quint16>(*reinterpret_cast<const quint16 *>(response.constData() + offset));
        offset += 2;

        if (type == DNS_TYPE_A && rdlength == 4) {
            if (offset + 4 > response.size()) {
                break;
            }

            QHostAddress ip;
            ip.setAddress(
                    qFromBigEndian<quint32>(*reinterpret_cast<const quint32 *>(response.constData() + offset)));
            return ip.toString();
        }

        offset += rdlength;
    }

    return QString();
}

} // namespace amnezia::transport::dns::detail
