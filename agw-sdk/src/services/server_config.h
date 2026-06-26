#ifndef AGW_SERVICES_SERVER_CONFIG_H
#define AGW_SERVICES_SERVER_CONFIG_H

#include <string>

#include "util/json.h"

namespace agw::services {

// Только gateway-типы (паритет с serverConfigUtils::configTypeFromJson в части шлюза).
// Self-hosted/native (с credentials / third-party) — НЕ дело SDK → Unknown (классифицирует приложение).
enum class GatewayConfigType {
    AmneziaFreeV2,
    AmneziaFreeV3,
    AmneziaPremiumV1,
    AmneziaPremiumV2,
    ExternalPremium,
    Unknown,
};

// Эндпоинты free-v2 / premium-v1 инъектируются (в клиенте это embedded FREE_V2/PREM_V1_ENDPOINT).
GatewayConfigType configTypeFromJson(const util::Json &serverConfig,
                                     const std::string &freeV2Endpoint,
                                     const std::string &premV1Endpoint);

} // namespace agw::services

#endif // AGW_SERVICES_SERVER_CONFIG_H
