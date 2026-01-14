#include "apiV2ServerConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/utils/api/apiUtils.h"
#include "core/models/api/apiConfig.h"
#include "core/models/api/authData.h"

namespace amnezia
{

using namespace ContainerEnumNS;

QString ApiV2ServerConfig::vpnKey() const
{
    if (!apiConfig.vpnKey.isEmpty()) {
        return apiConfig.vpnKey;
    }
    
    QJsonObject json = toJson();
    return apiUtils::getPremiumV2VpnKey(json);
}

QString ApiV2ServerConfig::serviceType() const
{
    return apiConfig.serviceType;
}

QString ApiV2ServerConfig::serviceProtocol() const
{
    return apiConfig.serviceProtocol;
}

bool ApiV2ServerConfig::isPremium() const
{
    return apiConfig.isPremium();
}

bool ApiV2ServerConfig::isFree() const
{
    return apiConfig.isFree();
}

bool ApiV2ServerConfig::isExternalPremium() const
{
    return apiConfig.isExternalPremium();
}

bool ApiV2ServerConfig::hasContainers() const
{
    return !containers.isEmpty();
}

ContainerConfig ApiV2ServerConfig::containerConfig(DockerContainer container) const
{
    if (!containers.contains(container)) {
        return ContainerConfig{};
    }
    return containers.value(container);
}

QJsonObject ApiV2ServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!name.isEmpty()) {
        obj[apiDefs::key::name] = name;
    }
    if (!description.isEmpty()) {
        obj[apiDefs::key::description] = description;
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
    
    QJsonObject apiConfigObj = apiConfig.toJson();
    if (!apiConfigObj.isEmpty()) {
        obj[apiDefs::key::apiConfig] = apiConfigObj;
    }
    
    QJsonObject authDataObj = authData.toJson();
    if (!authDataObj.isEmpty()) {
        obj[QLatin1String("auth_data")] = authDataObj;
    }
    
    return obj;
}

ApiV2ServerConfig ApiV2ServerConfig::fromJson(const QJsonObject& json)
{
    ApiV2ServerConfig config;
    
    config.name = json.value(apiDefs::key::name).toString();
    config.description = json.value(apiDefs::key::description).toString();
    config.configVersion = json.value(apiDefs::key::configVersion).toInt(2);
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
    
    QJsonObject apiConfigObj = json.value(apiDefs::key::apiConfig).toObject();
    if (!apiConfigObj.isEmpty()) {
        config.apiConfig = ApiConfig::fromJson(apiConfigObj);
    }
    
    QJsonObject authDataObj = json.value(QLatin1String("auth_data")).toObject();
    if (!authDataObj.isEmpty()) {
        config.authData = AuthData::fromJson(authDataObj);
    }
    
    return config;
}

} // namespace amnezia

