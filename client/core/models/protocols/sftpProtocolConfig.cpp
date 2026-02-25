#include "sftpProtocolConfig.h"

#include "../../../core/utils/protocolEnum.h"
#include "../../../core/protocols/protocolUtils.h"
#include "../../../core/utils/constants/configKeys.h"
#include "../../../core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;
namespace amnezia
{

QJsonObject SftpProtocolConfig::toJson() const
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

SftpProtocolConfig SftpProtocolConfig::fromJson(const QJsonObject& json)
{
    SftpProtocolConfig config;
    
    config.port = json.value(configKey::port).toString();
    config.userName = json.value(configKey::userName).toString();
    config.password = json.value(configKey::password).toString();
    
    return config;
}

} // namespace amnezia

