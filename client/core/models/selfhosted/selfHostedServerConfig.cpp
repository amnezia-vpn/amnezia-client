#include "selfHostedServerConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <stdexcept>

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

bool SelfHostedServerConfig::hasCredentials() const
{
    return userName.has_value() && password.has_value() && port.has_value();
}

bool SelfHostedServerConfig::isReadOnly() const
{
    return !hasCredentials();
}

std::optional<ServerCredentials> SelfHostedServerConfig::credentials() const
{
    if (!hasCredentials()) {
        return std::nullopt;
    }
    
    ServerCredentials creds;
    creds.hostName = hostName;
    creds.userName = userName.value();
    creds.secretData = password.value();
    creds.port = port.value();
    
    return creds;
}

bool SelfHostedServerConfig::hasContainers() const
{
    return !containers.isEmpty();
}

ContainerConfig SelfHostedServerConfig::containerConfig(DockerContainer container) const
{
    if (!containers.contains(container)) {
        return ContainerConfig{};
    }
    return containers.value(container);
}

QJsonObject SelfHostedServerConfig::toJson() const
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
    
    if (userName.has_value()) {
        obj[configKey::userName] = userName.value();
    }
    if (password.has_value()) {
        obj[configKey::password] = password.value();
    }
    if (port.has_value()) {
        obj[configKey::port] = port.value();
    }
    
    return obj;
}

SelfHostedServerConfig SelfHostedServerConfig::fromJson(const QJsonObject& json)
{
    SelfHostedServerConfig config;
    
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
    
    if (json.contains(configKey::userName)) {
        config.userName = json.value(configKey::userName).toString();
    }
    if (json.contains(configKey::password)) {
        config.password = json.value(configKey::password).toString();
    }
    if (json.contains(configKey::port)) {
        config.port = json.value(configKey::port).toInt();
    }
    
    return config;
}

} // namespace amnezia

