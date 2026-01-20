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

bool ServerConfigUtils::isSelfHosted(const ServerConfig& config)
{
    return std::holds_alternative<SelfHostedServerConfig>(config);
}

bool ServerConfigUtils::isNative(const ServerConfig& config)
{
    return std::holds_alternative<NativeServerConfig>(config);
}

bool ServerConfigUtils::isApiV1Config(const ServerConfig& config)
{
    return std::holds_alternative<ApiV1ServerConfig>(config);
}

bool ServerConfigUtils::isApiV2Config(const ServerConfig& config)
{
    return std::holds_alternative<ApiV2ServerConfig>(config);
}

bool ServerConfigUtils::isApiConfig(const ServerConfig& config)
{
    return isApiV1Config(config) || isApiV2Config(config);
}

SelfHostedServerConfig& ServerConfigUtils::asSelfHosted(ServerConfig& config)
{
    return std::get<SelfHostedServerConfig>(config);
}

const SelfHostedServerConfig& ServerConfigUtils::asSelfHosted(const ServerConfig& config)
{
    return std::get<SelfHostedServerConfig>(config);
}

NativeServerConfig& ServerConfigUtils::asNative(ServerConfig& config)
{
    return std::get<NativeServerConfig>(config);
}

const NativeServerConfig& ServerConfigUtils::asNative(const ServerConfig& config)
{
    return std::get<NativeServerConfig>(config);
}

ApiV1ServerConfig& ServerConfigUtils::asApiV1(ServerConfig& config)
{
    return std::get<ApiV1ServerConfig>(config);
}

const ApiV1ServerConfig& ServerConfigUtils::asApiV1(const ServerConfig& config)
{
    return std::get<ApiV1ServerConfig>(config);
}

ApiV2ServerConfig& ServerConfigUtils::asApiV2(ServerConfig& config)
{
    return std::get<ApiV2ServerConfig>(config);
}

const ApiV2ServerConfig& ServerConfigUtils::asApiV2(const ServerConfig& config)
{
    return std::get<ApiV2ServerConfig>(config);
}

QString ServerConfigUtils::description(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> QString {
        return arg.description;
    }, config);
}

QString ServerConfigUtils::hostName(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> QString {
        return arg.hostName;
    }, config);
}

QMap<DockerContainer, ContainerConfig> ServerConfigUtils::containers(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> QMap<DockerContainer, ContainerConfig> {
        return arg.containers;
    }, config);
}

DockerContainer ServerConfigUtils::defaultContainer(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> DockerContainer {
        return arg.defaultContainer;
    }, config);
}

QString ServerConfigUtils::dns1(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> QString {
        return arg.dns1;
    }, config);
}

QString ServerConfigUtils::dns2(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> QString {
        return arg.dns2;
    }, config);
}

bool ServerConfigUtils::hasContainers(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> bool {
        return arg.hasContainers();
    }, config);
}

ContainerConfig ServerConfigUtils::containerConfig(const ServerConfig& config, DockerContainer container)
{
    return std::visit([container](auto&& arg) -> ContainerConfig {
        return arg.containerConfig(container);
    }, config);
}

int ServerConfigUtils::crc(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> int {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, ApiV1ServerConfig> ||
                      std::is_same_v<std::decay_t<decltype(arg)>, ApiV2ServerConfig>) {
            return arg.crc;
        }
        return 0;
    }, config);
}

int ServerConfigUtils::configVersion(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> int {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ApiV1ServerConfig>) {
            return apiDefs::ConfigSource::Telegram;
        } else if constexpr (std::is_same_v<T, ApiV2ServerConfig>) {
            return apiDefs::ConfigSource::AmneziaGateway;
        }
        return 0; // SelfHostedServerConfig or NativeServerConfig
    }, config);
}

QJsonObject ServerConfigUtils::toJson(const ServerConfig& config)
{
    return std::visit([](auto&& arg) -> QJsonObject {
        return arg.toJson();
    }, config);
}

ServerConfig ServerConfigUtils::fromJson(const QJsonObject& json)
{
    apiDefs::ConfigType configType = apiUtils::getConfigType(json);
    
    switch (configType) {
    case apiDefs::ConfigType::SelfHosted: {
        bool hasCredentials = json.contains(config_key::userName) && 
                              json.contains(config_key::password) && 
                              json.contains(config_key::port);
        if (hasCredentials) {
            return SelfHostedServerConfig::fromJson(json);
        } else {
            return NativeServerConfig::fromJson(json);
        }
    }
    case apiDefs::ConfigType::AmneziaPremiumV1:
    case apiDefs::ConfigType::AmneziaFreeV2:
        return ApiV1ServerConfig::fromJson(json);
    case apiDefs::ConfigType::AmneziaPremiumV2:
    case apiDefs::ConfigType::AmneziaFreeV3:
    case apiDefs::ConfigType::ExternalPremium:
        return ApiV2ServerConfig::fromJson(json);
    default: {
        bool hasCredentials = json.contains(config_key::userName) && 
                              json.contains(config_key::password) && 
                              json.contains(config_key::port);
        if (hasCredentials) {
            return SelfHostedServerConfig::fromJson(json);
        } else {
            return NativeServerConfig::fromJson(json);
        }
    }
    }
}

QPair<QString, QString> ServerConfigUtils::getDnsPair(const ServerConfig &serverConfig, 
                                                       bool isAmneziaDnsEnabled,
                                                       const QString &primaryDns,
                                                       const QString &secondaryDns)
{
    QPair<QString, QString> dns;
    
    QMap<DockerContainer, ContainerConfig> serverContainers = ServerConfigUtils::containers(serverConfig);
    
    bool isDnsContainerInstalled = false;
    for (auto it = serverContainers.begin(); it != serverContainers.end(); ++it) {
        if (it.key() == DockerContainer::Dns) {
            isDnsContainerInstalled = true;
            break;
        }
    }
    
    dns.first = ServerConfigUtils::dns1(serverConfig);
    dns.second = ServerConfigUtils::dns2(serverConfig);
    
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

