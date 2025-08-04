#include "ikev2ProtocolConfig.h"

#include "config_key.h"

Ikev2ProtocolConfig::Ikev2ProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName)
    : ProtocolConfig(protocolName)
{
    Q_UNUSED(protocolConfigObject)
}

Ikev2ProtocolConfig::Ikev2ProtocolConfig(const QString &protocolName)
    : ProtocolConfig(protocolName)
{
}

QJsonObject Ikev2ProtocolConfig::toJson() const
{
    QJsonObject protocolConfigObject;
    
    return protocolConfigObject;
}

bool Ikev2ProtocolConfig::hasEqualServerSettings(const Ikev2ProtocolConfig &other) const
{
    Q_UNUSED(other)
    return true;
}

void Ikev2ProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = ikev2::ClientProtocolConfig();
}
