#include "torProtocolConfig.h"

#include "core/utils/constants/configKeys.h"

using namespace amnezia;
using namespace config_key;

namespace amnezia
{

QJsonObject TorServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!site.isEmpty()) {
        obj[config_key::site] = site;
    }
    
    return obj;
}

TorServerConfig TorServerConfig::fromJson(const QJsonObject& json)
{
    TorServerConfig config;
    
    config.site = json.value(config_key::site).toString();
    
    return config;
}

QJsonObject TorProtocolConfig::toJson() const
{
    return serverConfig.toJson();
}

TorProtocolConfig TorProtocolConfig::fromJson(const QJsonObject& json)
{
    TorProtocolConfig config;
    
    config.serverConfig = TorServerConfig::fromJson(json);
    
    return config;
}

} // namespace amnezia

