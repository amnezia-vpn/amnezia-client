#include "socks5ProxyProtocolConfig.h"

#include "../../../protocols/protocols_defs.h"

namespace amnezia
{

QJsonObject Socks5ProxyProtocolConfig::toJson() const
{
    QJsonObject obj;
    
    if (!port.isEmpty()) {
        obj[config_key::port] = port;
    }
    if (!userName.isEmpty()) {
        obj[config_key::userName] = userName;
    }
    if (!password.isEmpty()) {
        obj[config_key::password] = password;
    }
    
    return obj;
}

Socks5ProxyProtocolConfig Socks5ProxyProtocolConfig::fromJson(const QJsonObject& json)
{
    Socks5ProxyProtocolConfig config;
    
    config.port = json.value(config_key::port).toString();
    config.userName = json.value(config_key::userName).toString();
    config.password = json.value(config_key::password).toString();
    
    return config;
}

} // namespace amnezia

