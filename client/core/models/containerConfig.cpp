#include "containerConfig.h"

#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "core/protocols/protocolsDefs.h"

namespace amnezia
{

using namespace ContainerEnumNS;
using namespace ProtocolEnumNS;

Proto ContainerConfig::getProtocolType() const
{
    return ContainerProps::defaultProtocol(container);
}

QJsonObject ContainerConfig::toJson() const
{
    QJsonObject obj;
    
    obj[config_key::container] = ContainerProps::containerToString(container);
    
    Proto protoType = getProtocolType();
    QString protoName = ProtocolProps::protoToString(protoType);
    
    QJsonObject protoJson = ProtocolConfigUtils::toJson(protocolConfig, protoType);
    
    obj[protoName] = protoJson;
    
    return obj;
}

ContainerConfig ContainerConfig::fromJson(const QJsonObject& json)
{
    ContainerConfig config;
    
    QString containerStr = json.value(config_key::container).toString();
    config.container = ContainerProps::containerFromString(containerStr);
    
    Proto protoType = ContainerProps::defaultProtocol(config.container);
    QString protoName = ProtocolProps::protoToString(protoType);
    
    QJsonObject protoJson = json.value(protoName).toObject();
    
    config.protocolConfig = ProtocolConfigUtils::fromJson(protoJson, protoType);
    
    return config;
}

} // namespace amnezia

