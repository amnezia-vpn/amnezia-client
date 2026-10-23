#include "socks5ProxyProtocolConfig.h"

#include "../../../core/utils/protocolEnum.h"
#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;
namespace amnezia
{

QJsonObject Socks5ProxyProtocolConfig::toJson() const
{
    QJsonObject obj;
    
    if (!port.isEmpty()) {
        obj[configKey::port] = port;
    }
    if (!userName.isEmpty()) {
        obj[configKey::userName] = userName;
    }
    if (!password.isEmpty()) {
        obj[configKey::password] = password;
    }
    
    return obj;
}

Socks5ProxyProtocolConfig Socks5ProxyProtocolConfig::fromJson(const QJsonObject& json)
{
    Socks5ProxyProtocolConfig config;
    
    config.port = json.value(configKey::port).toString();
    config.userName = json.value(configKey::userName).toString();
    config.password = json.value(configKey::password).toString();
    
    return config;
}

} // namespace amnezia

