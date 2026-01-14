#include "apiV1ServerConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
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
        obj[apiDefs::key::name] = name;
    }
    if (!description.isEmpty()) {
        obj[apiDefs::key::description] = description;
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
    
    obj[apiDefs::key::configVersion] = configVersion;
    
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
        obj[config_key::defaultContainer] = ContainerProps::containerToString(defaultContainer);
    }
    
    if (!dns1.isEmpty()) {
        obj[config_key::dns1] = dns1;
    }
    if (!dns2.isEmpty()) {
        obj[config_key::dns2] = dns2;
    }
    
    if (crc > 0) {
        obj[config_key::crc] = crc;
    }
    
    return obj;
}

ApiV1ServerConfig ApiV1ServerConfig::fromJson(const QJsonObject& json)
{
    ApiV1ServerConfig config;
    
    config.name = json.value(apiDefs::key::name).toString();
    config.description = json.value(apiDefs::key::description).toString();
    config.protocol = json.value(apiDefs::key::protocol).toString();
    config.apiEndpoint = json.value(apiDefs::key::apiEndpoint).toString();
    config.apiKey = json.value(apiDefs::key::apiKey).toString();
    config.configVersion = json.value(apiDefs::key::configVersion).toInt(1);
    config.hostName = json.value(config_key::hostName).toString();
    
    QJsonArray containersArray = json.value(config_key::containers).toArray();
    for (const QJsonValue& val : containersArray) {
        QJsonObject containerObj = val.toObject();
        ContainerConfig containerConfig = ContainerConfig::fromJson(containerObj);
        
        QString containerStr = containerObj.value(config_key::container).toString();
        DockerContainer container = ContainerProps::containerFromString(containerStr);
        
        config.containers.insert(container, containerConfig);
    }
    
    QString defaultContainerStr = json.value(config_key::defaultContainer).toString();
    config.defaultContainer = ContainerProps::containerFromString(defaultContainerStr);
    
    config.dns1 = json.value(config_key::dns1).toString();
    config.dns2 = json.value(config_key::dns2).toString();
    
    config.crc = json.value(config_key::crc).toInt(0);
    
    return config;
}

} // namespace amnezia

