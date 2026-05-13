#include "legacyApiServerConfig.h"

#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"

namespace amnezia
{

bool LegacyApiServerConfig::hasContainers() const
{
    return !containers.isEmpty();
}

ContainerConfig LegacyApiServerConfig::containerConfig(DockerContainer container) const
{
    if (!containers.contains(container)) {
        return ContainerConfig{};
    }
    return containers.value(container);
}

QJsonObject LegacyApiServerConfig::toJson() const
{
    QJsonObject obj;

    if (!name.isEmpty()) {
        obj[configKey::name] = name;
    }
    if (!description.isEmpty()) {
        obj[configKey::description] = description;
    }
    if (!displayName.isEmpty()) {
        obj[configKey::displayName] = displayName;
    }

    obj[configKey::configVersion] = configVersion;

    if (!apiEndpoint.isEmpty()) {
        obj[apiDefs::key::apiEndpoint] = apiEndpoint;
    }

    if (!hostName.isEmpty()) {
        obj[configKey::hostName] = hostName;
    }

    if (crc > 0) {
        obj[configKey::crc] = crc;
    }

    return obj;
}

LegacyApiServerConfig LegacyApiServerConfig::fromJson(const QJsonObject &json)
{
    LegacyApiServerConfig config;

    config.name = json.value(configKey::name).toString();
    config.description = json.value(configKey::description).toString();
    config.displayName = json.value(configKey::displayName).toString();
    config.hostName = json.value(configKey::hostName).toString();

    config.crc = json.value(configKey::crc).toInt(0);

    config.configVersion = json.value(configKey::configVersion).toInt(1);
    config.apiEndpoint = json.value(apiDefs::key::apiEndpoint).toString();

    if (config.displayName.isEmpty()) {
        config.displayName = config.name.isEmpty() ? config.description : config.name;
    }

    return config;
}

} // namespace amnezia
