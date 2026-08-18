#include "serverConfigUtils.h"

#include <QJsonArray>
#include <QJsonValue>

#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"

namespace
{

bool hasThirdPartyConfig(const QJsonObject &json)
{
    const QJsonArray containersArray = json.value(amnezia::configKey::containers).toArray();
    for (const QJsonValue &val : containersArray) {
        const QJsonObject containerObj = val.toObject();
        for (auto it = containerObj.begin(); it != containerObj.end(); ++it) {
            if (it.key() == amnezia::configKey::container) {
                continue;
            }
            const QJsonObject protocolObj = it.value().toObject();
            if (protocolObj.contains(amnezia::configKey::isThirdPartyConfig)
                && protocolObj.value(amnezia::configKey::isThirdPartyConfig).toBool()) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

namespace serverConfigUtils
{

bool isServerFromApi(const QJsonObject &serverConfigObject)
{
    const int configVersion = serverConfigObject.value(amnezia::configKey::configVersion).toInt();
    switch (configVersion) {
    case ConfigSource::Telegram:
    case ConfigSource::AmneziaGateway:
        return true;
    default:
        return false;
    }
}

ConfigSource getConfigSource(const QJsonObject &serverConfigObject)
{
    return static_cast<ConfigSource>(serverConfigObject.value(amnezia::configKey::configVersion).toInt());
}

ConfigType configTypeFromJson(const QJsonObject &serverConfigObject)
{
    const int configVersion = serverConfigObject.value(amnezia::configKey::configVersion).toInt();

    switch (configVersion) {
    case ConfigSource::Telegram: {
        constexpr QLatin1String freeV2Endpoint(FREE_V2_ENDPOINT);
        constexpr QLatin1String premiumV1Endpoint(PREM_V1_ENDPOINT);

        const QString apiEndpointValue = serverConfigObject.value(apiDefs::key::apiEndpoint).toString();

        if (apiEndpointValue.contains(premiumV1Endpoint)) {
            return ConfigType::AmneziaPremiumV1;
        }
        if (apiEndpointValue.contains(freeV2Endpoint)) {
            return ConfigType::AmneziaFreeV2;
        }
    }
        [[fallthrough]];
    case ConfigSource::AmneziaGateway: {
        constexpr QLatin1String servicePremium("amnezia-premium");
        constexpr QLatin1String serviceFree("amnezia-free");
        constexpr QLatin1String serviceExternalPremium("external-premium");

        const QJsonObject apiConfigObject = serverConfigObject.value(apiDefs::key::apiConfig).toObject();
        const QString serviceTypeStr = apiConfigObject.value(apiDefs::key::serviceType).toString();

        if (serviceTypeStr == servicePremium) {
            return ConfigType::AmneziaPremiumV2;
        }
        if (serviceTypeStr == serviceFree) {
            return ConfigType::AmneziaFreeV3;
        }
        if (serviceTypeStr == serviceExternalPremium) {
            return ConfigType::ExternalPremium;
        }
        break;
    }
    default:
        break;
    }

    if (hasThirdPartyConfig(serverConfigObject)) {
        return ConfigType::Native;
    }

    const amnezia::SelfHostedAdminServerConfig adminProbe =
            amnezia::SelfHostedAdminServerConfig::fromJson(serverConfigObject);
    return adminProbe.hasCredentials() ? ConfigType::SelfHostedAdmin : ConfigType::SelfHostedUser;
}

bool isLegacyApiSubscription(ConfigType configType)
{
    return configType == ConfigType::AmneziaPremiumV1 || configType == ConfigType::AmneziaFreeV2;
}

bool isApiV2Subscription(ConfigType configType)
{
    switch (configType) {
    case ConfigType::AmneziaPremiumV2:
    case ConfigType::AmneziaFreeV3:
    case ConfigType::ExternalPremium:
        return true;
    default:
        return false;
    }
}

} // namespace serverConfigUtils
