#ifndef DNSPROTOCOLCONFIG_H
#define DNSPROTOCOLCONFIG_H

#include <QJsonObject>

namespace amnezia
{

struct DnsProtocolConfig {
    QJsonObject toJson() const;
    static DnsProtocolConfig fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // DNSPROTOCOLCONFIG_H

