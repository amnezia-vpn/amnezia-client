#include "xraySubscriptionConfig.h"

#include <QJsonArray>

#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"

namespace amnezia
{

    using namespace ContainerEnumNS;

    bool XRaySubscriptionConfig::hasContainers() const
    {
        return !containers.isEmpty();
    }

    ContainerConfig XRaySubscriptionConfig::containerConfig(DockerContainer container) const
    {
        if (!containers.contains(container)) {
            return ContainerConfig {};
        }
        return containers.value(container);
    }

    QJsonObject XRaySubscriptionConfig::toJson() const
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

        if (!configString.isEmpty()) {
            obj[configKey::xraySubscriptionConfig] = configString;
        }
        if (!configName.isEmpty()) {
            obj[configKey::xraySubscriptionConfigName] = configName;
        }
        if (currentConfig > -1) {
            obj[configKey::xraySubscriptionConfigCurrent] = currentConfig;
        }

        return obj;
    }

    XRaySubscriptionConfig XRaySubscriptionConfig::fromJson(const QJsonObject &json)
    {
        XRaySubscriptionConfig config;

        config.description = json.value(configKey::description).toString();
        config.hostName = json.value(configKey::hostName).toString();

        QJsonArray containersArray = json.value(configKey::containers).toArray();
        for (const QJsonValue &val : containersArray) {
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

        config.configString = json.value(configKey::xraySubscriptionConfig).toArray();
        config.configName = json.value(configKey::xraySubscriptionConfigName).toArray();
        config.currentConfig = json.value(configKey::xraySubscriptionConfigCurrent).toInt();

        return config;
    }

} // namespace amnezia
