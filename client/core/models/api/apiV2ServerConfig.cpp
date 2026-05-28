#include "apiV2ServerConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/models/api/apiConfig.h"
#include "core/models/api/authData.h"
#include "core/utils/networkUtilities.h"

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

QPair<QString, QString> ApiV2ServerConfig::getDnsPair(const QString &primaryDns, const QString &secondaryDns) const
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

QJsonObject ApiV2ServerConfig::toJson() const
{
    QJsonObject obj;
    
    if (!name.isEmpty()) {
        obj[configKey::name] = name;
    }
    if (nameOverriddenByUser) {
        obj[configKey::nameOverriddenByUser] = true;
    }
    if (!description.isEmpty()) {
        obj[configKey::description] = description;
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
    
    config.name = json.value(configKey::name).toString();
    config.nameOverriddenByUser = json.value(configKey::nameOverriddenByUser).toBool(false);
    config.description = json.value(configKey::description).toString();
    config.configVersion = json.value(configKey::configVersion).toInt(2);
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
    
    QJsonObject apiConfigObj = json.value(apiDefs::key::apiConfig).toObject();
    if (!apiConfigObj.isEmpty()) {
        config.apiConfig = ApiConfig::fromJson(apiConfigObj);
    }
    
    QJsonObject authDataObj = json.value(QLatin1String("auth_data")).toObject();
    if (!authDataObj.isEmpty()) {
        config.authData = AuthData::fromJson(authDataObj);
    }
    
    if (config.displayName.isEmpty()) {
        config.displayName = config.name.isEmpty() ? config.description : config.name;
    }
    
    return config;
}

} // namespace amnezia

