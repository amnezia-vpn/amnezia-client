#include "serverConfig.h"

#include "core/utils/api/apiUtils.h"
#include "core/utils/networkUtilities.h"
#include "core/models/selfhosted/selfHostedServerConfig.h"
#include "core/models/selfhosted/nativeServerConfig.h"
#include "core/models/api/apiV1ServerConfig.h"
#include "core/models/api/apiV2ServerConfig.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

namespace amnezia
{

using namespace ContainerEnumNS;

QString ServerConfig::description() const
{
    return std::visit([](const auto& v) { return v.description; }, data);
}

QString ServerConfig::hostName() const
{
    return std::visit([](const auto& v) { return v.hostName; }, data);
}

QString ServerConfig::displayName() const
{
    if (isApiV1()) {
        const auto *apiV1 = as<ApiV1ServerConfig>();
        return apiV1 ? apiV1->name : description();
    }
    if (isApiV2()) {
        const auto *apiV2 = as<ApiV2ServerConfig>();
        return apiV2 ? apiV2->name : description();
    }
    QString name = description();
    return name.isEmpty() ? hostName() : name;
}

QMap<DockerContainer, ContainerConfig> ServerConfig::containers() const
{
    return std::visit([](const auto& v) { return v.containers; }, data);
}

DockerContainer ServerConfig::defaultContainer() const
{
    return std::visit([](const auto& v) { return v.defaultContainer; }, data);
}

QString ServerConfig::dns1() const
{
    return std::visit([](const auto& v) { return v.dns1; }, data);
}

QString ServerConfig::dns2() const
{
    return std::visit([](const auto& v) { return v.dns2; }, data);
}

bool ServerConfig::hasContainers() const
{
    return std::visit([](const auto& v) { return v.hasContainers(); }, data);
}

ContainerConfig ServerConfig::containerConfig(DockerContainer container) const
{
    return std::visit([container](const auto& v) { return v.containerConfig(container); }, data);
}

int ServerConfig::crc() const
{
    return std::visit([](const auto& v) -> int {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, ApiV1ServerConfig> ||
                      std::is_same_v<T, ApiV2ServerConfig>) {
            return v.crc;
        }
        return 0;
    }, data);
}

int ServerConfig::configVersion() const
{
    return std::visit([](const auto& v) -> int {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, ApiV1ServerConfig>) {
            return apiDefs::ConfigSource::Telegram;
        } else if constexpr (std::is_same_v<T, ApiV2ServerConfig>) {
            return apiDefs::ConfigSource::AmneziaGateway;
        }
        return 0; // SelfHostedServerConfig or NativeServerConfig
    }, data);
}

bool ServerConfig::isSelfHosted() const
{
    return std::holds_alternative<SelfHostedServerConfig>(data);
}

bool ServerConfig::isNative() const
{
    return std::holds_alternative<NativeServerConfig>(data);
}

bool ServerConfig::isApiV1() const
{
    return std::holds_alternative<ApiV1ServerConfig>(data);
}

bool ServerConfig::isApiV2() const
{
    return std::holds_alternative<ApiV2ServerConfig>(data);
}

bool ServerConfig::isApiConfig() const
{
    return isApiV1() || isApiV2();
}

QJsonObject ServerConfig::toJson() const
{
    return std::visit([](const auto& v) { return v.toJson(); }, data);
}

ServerConfig ServerConfig::fromJson(const QJsonObject& json)
{
    apiDefs::ConfigType configType = apiUtils::getConfigType(json);
    
    switch (configType) {
    case apiDefs::ConfigType::SelfHosted: {
        bool hasThirdPartyConfig = false;
        QJsonArray containersArray = json.value(configKey::containers).toArray();
        for (const QJsonValue& val : containersArray) {
            QJsonObject containerObj = val.toObject();
            for (auto it = containerObj.begin(); it != containerObj.end(); ++it) {
                QString key = it.key();
                if (key != configKey::container) {
                    QJsonObject protocolObj = it.value().toObject();
                    if (protocolObj.contains(configKey::isThirdPartyConfig) && 
                        protocolObj.value(configKey::isThirdPartyConfig).toBool()) {
                        hasThirdPartyConfig = true;
                        break;
                    }
                }
            }
            if (hasThirdPartyConfig) {
                break;
            }
        }
        
        if (hasThirdPartyConfig) {
            return ServerConfig{NativeServerConfig::fromJson(json)};
        } else {
            return ServerConfig{SelfHostedServerConfig::fromJson(json)};
        }
    }
    case apiDefs::ConfigType::AmneziaPremiumV1:
    case apiDefs::ConfigType::AmneziaFreeV2:
        return ServerConfig{ApiV1ServerConfig::fromJson(json)};
    case apiDefs::ConfigType::AmneziaPremiumV2:
    case apiDefs::ConfigType::AmneziaFreeV3:
    case apiDefs::ConfigType::ExternalPremium:
        return ServerConfig{ApiV2ServerConfig::fromJson(json)};
    default: {
        // Check if any container has isThirdPartyConfig
        bool hasThirdPartyConfig = false;
        QJsonArray containersArray = json.value(configKey::containers).toArray();
        for (const QJsonValue& val : containersArray) {
            QJsonObject containerObj = val.toObject();
            // Check all protocol keys in the container object
            for (auto it = containerObj.begin(); it != containerObj.end(); ++it) {
                QString key = it.key();
                if (key != configKey::container) {
                    QJsonObject protocolObj = it.value().toObject();
                    if (protocolObj.contains(configKey::isThirdPartyConfig) && 
                        protocolObj.value(configKey::isThirdPartyConfig).toBool()) {
                        hasThirdPartyConfig = true;
                        break;
                    }
                }
            }
            if (hasThirdPartyConfig) {
                break;
            }
        }
        
        if (hasThirdPartyConfig) {
            return ServerConfig{NativeServerConfig::fromJson(json)};
        } else {
            return ServerConfig{SelfHostedServerConfig::fromJson(json)};
        }
    }
    }
}

QPair<QString, QString> ServerConfig::getDnsPair(bool isAmneziaDnsEnabled,
                                                 const QString &primaryDns,
                                                 const QString &secondaryDns) const
{
    QPair<QString, QString> dns;
    
    QMap<DockerContainer, ContainerConfig> serverContainers = containers();
    
    bool isDnsContainerInstalled = false;
    for (auto it = serverContainers.begin(); it != serverContainers.end(); ++it) {
        if (it.key() == DockerContainer::Dns) {
            isDnsContainerInstalled = true;
            break;
        }
    }
    
    dns.first = dns1();
    dns.second = dns2();
    
    if (dns.first.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.first)) {
        if (isAmneziaDnsEnabled && isDnsContainerInstalled) {
            dns.first = protocols::dns::amneziaDnsIp;
        } else {
            dns.first = primaryDns;
        }
    }
    
    if (dns.second.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.second)) {
        dns.second = secondaryDns;
    }
    
    return dns;
}

} // namespace amnezia

