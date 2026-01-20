#include "sftpProtocolConfig.h"

#include "../../../core/protocols/protocolsDefs.h"

namespace amnezia
{

QJsonObject SftpProtocolConfig::toJson() const
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

SftpProtocolConfig SftpProtocolConfig::fromJson(const QJsonObject& json)
{
    SftpProtocolConfig config;
    
    config.port = json.value(config_key::port).toString();
    config.userName = json.value(config_key::userName).toString();
    config.password = json.value(config_key::password).toString();
    
    return config;
}

} // namespace amnezia

