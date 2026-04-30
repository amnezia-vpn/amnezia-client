#include "apiV1ServerConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/api/apiUtils.h"

namespace amnezia
{

using namespace ContainerEnumNS;

bool ApiV1ServerConfig::isPremium() const
{
    constexpr QLatin1String premiumV1Endpoint(PREM_V1_ENDPOINT);
    return apiEndpoint.contains(premiumV1Endpoint);
}

bool ApiV1ServerConfig::isFree() const
{
    constexpr QLatin1String freeV2Endpoint(FREE_V2_ENDPOINT);
    return apiEndpoint.contains(freeV2Endpoint);
}

QString ApiV1ServerConfig::vpnKey() const
{
    QJsonObject json = toJson();
    return apiUtils::getPremiumV1VpnKey(json);
}

bool ApiV1ServerConfig::hasContainers() const
{
    return !containers.isEmpty();
}

ContainerConfig ApiV1ServerConfig::containerConfig(DockerContainer container) const
{
    if (!containers.contains(container)) {
        return ContainerConfig{};
    }
    return containers.value(container);
}

QJsonObject ApiV1ServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!name.isEmpty()) {
        obj[configKey::name] = name;
    }
    if (!description.isEmpty()) {
        obj[configKey::description] = description;
    }
    if (!protocol.isEmpty()) {
        obj[apiDefs::key::protocol] = protocol;
    }
    if (!apiEndpoint.isEmpty()) {
        obj[apiDefs::key::apiEndpoint] = apiEndpoint;
    }
    if (!apiKey.isEmpty()) {
        obj[apiDefs::key::apiKey] = apiKey;
    }
    
    obj[configKey::configVersion] = configVersion;
    
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
    
    if (crc > 0) {
        obj[configKey::crc] = crc;
    }
    
    return obj;
}

ApiV1ServerConfig ApiV1ServerConfig::fromJson(const QJsonObject& json)
{
    ApiV1ServerConfig config;
    
    config.name = json.value(configKey::name).toString();
    config.description = json.value(configKey::description).toString();
    config.protocol = json.value(apiDefs::key::protocol).toString();
    config.apiEndpoint = json.value(apiDefs::key::apiEndpoint).toString();
    config.apiKey = json.value(apiDefs::key::apiKey).toString();
    config.configVersion = json.value(configKey::configVersion).toInt(1);
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
    
    config.crc = json.value(configKey::crc).toInt(0);
    
    return config;
}

} // namespace amnezia

