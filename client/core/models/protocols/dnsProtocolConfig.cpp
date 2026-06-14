#include "dnsProtocolConfig.h"

namespace amnezia
{

QJsonObject DnsProtocolConfig::toJson() const
{
    return QJsonObject();
}

DnsProtocolConfig DnsProtocolConfig::fromJson(const QJsonObject& json)
{
    Q_UNUSED(json);
    return DnsProtocolConfig();
}

} // namespace amnezia

