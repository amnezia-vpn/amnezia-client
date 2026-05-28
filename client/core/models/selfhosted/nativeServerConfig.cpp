#include "nativeServerConfig.h"

#include <QJsonArray>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/networkUtilities.h"

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

QPair<QString, QString> NativeServerConfig::getDnsPair(const QString &primaryDns, const QString &secondaryDns) const
{
    QString d1 = dns1;
    QString d2 = dns2;

    if (d1.isEmpty() || !NetworkUtilities::checkIPv4Format(d1)) {
        d1 = primaryDns;
    }
    if (d2.isEmpty() || !NetworkUtilities::checkIPv4Format(d2)) {
        d2 = secondaryDns;
    }
    return { d1, d2 };
}

QJsonObject NativeServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!description.isEmpty()) {
        obj[configKey::description] = this->description;
    }
    if (!hostName.isEmpty()) {
        obj[configKey::hostName] = hostName;
    }
    
    QJsonArray containersArray;
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        QJsonObject containerObj = it.value().toJson();
        containersArray.append(containerObj);
    }
    if (!containersArray.isEmpty()) {
        obj[configKey::containers] = containersArray;
    }
    
    if (defaultContainer != DockerContainer::None) {
        obj[configKey::defaultContainer] = ContainerUtils::containerToString(defaultContainer);
    }
    
    if (!dns1.isEmpty()) {
        obj[configKey::dns1] = dns1;
    }
    if (!dns2.isEmpty()) {
        obj[configKey::dns2] = dns2;
    }
    
    return obj;
}

NativeServerConfig NativeServerConfig::fromJson(const QJsonObject& json)
{
    NativeServerConfig config;
    
    config.description = json.value(configKey::description).toString();
    config.hostName = json.value(configKey::hostName).toString();
    
    QJsonArray containersArray = json.value(configKey::containers).toArray();
    for (const QJsonValue& val : containersArray) {
        QJsonObject containerObj = val.toObject();
        ContainerConfig containerConfig = ContainerConfig::fromJson(containerObj);
        
        QString containerStr = containerObj.value(configKey::container).toString();
        DockerContainer container = ContainerUtils::containerFromString(containerStr);
        
        config.containers.insert(container, containerConfig);
    }
    
    QString defaultContainerStr = json.value(configKey::defaultContainer).toString();
    config.defaultContainer = ContainerUtils::containerFromString(defaultContainerStr);
    
    config.dns1 = json.value(configKey::dns1).toString();
    config.dns2 = json.value(configKey::dns2).toString();
    
    if (config.displayName.isEmpty()) {
        config.displayName = config.description.isEmpty() ? config.hostName : config.description;
    }
    
    return config;
}

} // namespace amnezia

