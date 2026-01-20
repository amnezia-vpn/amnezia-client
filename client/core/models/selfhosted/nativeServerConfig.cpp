#include "nativeServerConfig.h"

#include <QJsonArray>

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

bool NativeServerConfig::hasContainers() const
{
    return !containers.isEmpty();
}

ContainerConfig NativeServerConfig::containerConfig(DockerContainer container) const
{
    if (!containers.contains(container)) {
        return ContainerConfig{};
    }
    return containers.value(container);
}

QJsonObject NativeServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!description.isEmpty()) {
        obj[config_key::description] = this->description;
    }
    if (!hostName.isEmpty()) {
        obj[config_key::hostName] = hostName;
    }
    
    QJsonArray containersArray;
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        QJsonObject containerObj = it.value().toJson();
        containersArray.append(containerObj);
    }
    if (!containersArray.isEmpty()) {
        obj[config_key::containers] = containersArray;
    }
    
    if (defaultContainer != DockerContainer::None) {
        obj[config_key::defaultContainer] = ContainerUtils::containerToString(defaultContainer);
    }
    
    if (!dns1.isEmpty()) {
        obj[config_key::dns1] = dns1;
    }
    if (!dns2.isEmpty()) {
        obj[config_key::dns2] = dns2;
    }
    
    return obj;
}

NativeServerConfig NativeServerConfig::fromJson(const QJsonObject& json)
{
    NativeServerConfig config;
    
    config.description = json.value(config_key::description).toString();
    config.hostName = json.value(config_key::hostName).toString();
    
    QJsonArray containersArray = json.value(config_key::containers).toArray();
    for (const QJsonValue& val : containersArray) {
        QJsonObject containerObj = val.toObject();
        ContainerConfig containerConfig = ContainerConfig::fromJson(containerObj);
        
        QString containerStr = containerObj.value(config_key::container).toString();
        DockerContainer container = ContainerUtils::containerFromString(containerStr);
        
        config.containers.insert(container, containerConfig);
    }
    
    QString defaultContainerStr = json.value(config_key::defaultContainer).toString();
    config.defaultContainer = ContainerUtils::containerFromString(defaultContainerStr);
    
    config.dns1 = json.value(config_key::dns1).toString();
    config.dns2 = json.value(config_key::dns2).toString();
    
    return config;
}

} // namespace amnezia

