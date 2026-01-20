#include "containerConfig.h"

#include <QJsonDocument>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

namespace amnezia
{

using namespace ContainerEnumNS;
using namespace ProtocolEnumNS;
using namespace ProtocolUtils;

Proto ContainerConfig::getProtocolType() const
{
    return ContainerUtils::defaultProtocol(container);
}

QJsonObject ContainerConfig::toJson() const
{
    QJsonObject obj;
    
    obj[config_key::container] = ContainerUtils::containerToString(container);
    
    Proto protoType = getProtocolType();
    QString protoName = ProtocolUtils::protoToString(protoType);
    
    QJsonObject protoJson = ProtocolConfigUtils::toJson(protocolConfig, protoType);
    
    obj[protoName] = protoJson;
    
    return obj;
}

ContainerConfig ContainerConfig::fromJson(const QJsonObject& json)
{
    ContainerConfig config;
    
    QString containerStr = json.value(config_key::container).toString();
    config.container = ContainerUtils::containerFromString(containerStr);
    
    Proto protoType = ContainerUtils::defaultProtocol(config.container);
    QString protoName = ProtocolUtils::protoToString(protoType);
    
    QJsonObject protoJson = json.value(protoName).toObject();
    
    config.protocolConfig = ProtocolConfigUtils::fromJson(protoJson, protoType);
    
    return config;
}

} // namespace amnezia

