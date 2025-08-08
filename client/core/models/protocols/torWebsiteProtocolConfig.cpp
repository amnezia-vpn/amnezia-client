#include "torWebsiteProtocolConfig.h"

#include "protocols/protocols_defs.h"

using namespace amnezia;

TorWebsiteProtocolConfig::TorWebsiteProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName)
    : ProtocolConfig(protocolName)
{
    serverProtocolConfig.site = protocolConfigObject.value(config_key::site).toString();
    
    clientProtocolConfig.isEmpty = false;
}

QJsonObject TorWebsiteProtocolConfig::toJson() const
{
    QJsonObject json;
    
    json[config_key::site] = serverProtocolConfig.site;
    
    return json;
}

bool TorWebsiteProtocolConfig::hasEqualServerSettings(const TorWebsiteProtocolConfig &other) const
{
    return serverProtocolConfig.site == other.serverProtocolConfig.site;
}

void TorWebsiteProtocolConfig::clearClientSettings()
{
    clientProtocolConfig.isEmpty = true;
}
