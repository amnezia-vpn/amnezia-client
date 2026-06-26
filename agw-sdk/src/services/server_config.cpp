#include "services/server_config.h"

#include "protocol/keys.h"

namespace agw::services {

namespace {
namespace k = protocol::keys;

// ConfigSource из serverConfigUtils: Telegram=1, AmneziaGateway=2.
constexpr int kSourceTelegram = 1;
constexpr int kSourceGateway = 2;

std::string str(const util::Json &j, const char *key)
{
    auto it = j.find(key);
    return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
}

int integer(const util::Json &j, const char *key)
{
    auto it = j.find(key);
    return (it != j.end() && it->is_number_integer()) ? it->get<int>() : 0;
}

} // namespace

GatewayConfigType configTypeFromJson(const util::Json &serverConfig,
                                     const std::string &freeV2Endpoint,
                                     const std::string &premV1Endpoint)
{
    if (!serverConfig.is_object()) {
        return GatewayConfigType::Unknown;
    }
    const int configVersion = integer(serverConfig, k::configVersion);

    if (configVersion == kSourceTelegram) {
        const std::string apiEndpoint = str(serverConfig, k::apiEndpoint);
        if (!premV1Endpoint.empty() && apiEndpoint.find(premV1Endpoint) != std::string::npos) {
            return GatewayConfigType::AmneziaPremiumV1;
        }
        if (!freeV2Endpoint.empty() && apiEndpoint.find(freeV2Endpoint) != std::string::npos) {
            return GatewayConfigType::AmneziaFreeV2;
        }
        // fallthrough: пробуем как gateway (по service_type), как в оригинале
    }

    if (configVersion == kSourceTelegram || configVersion == kSourceGateway) {
        util::Json apiConfig = serverConfig.contains(k::apiConfig) && serverConfig.at(k::apiConfig).is_object()
                ? serverConfig.at(k::apiConfig)
                : util::Json::object();
        const std::string serviceType = str(apiConfig, k::serviceType);
        if (serviceType == "amnezia-premium") {
            return GatewayConfigType::AmneziaPremiumV2;
        }
        if (serviceType == "amnezia-free") {
            return GatewayConfigType::AmneziaFreeV3;
        }
        if (serviceType == "external-premium") {
            return GatewayConfigType::ExternalPremium;
        }
    }

    // self-hosted/native — не дело SDK
    return GatewayConfigType::Unknown;
}

} // namespace agw::services
