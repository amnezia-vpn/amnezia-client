#ifndef AGW_MODELS_H
#define AGW_MODELS_H

#include <string>

// Публичные Qt-free / nlohmann-free модели ответов API (Tier 2). Только std-типы — наружу не течёт
// ни nlohmann, ни Qt. Парсинг — внутри SDK (src/services). UI-модели (списки стран и т.п.) строит
// приложение из этих структур (Tier 3).
namespace agw {

struct ServiceInfo {
    bool isAdVisible = false;
    bool isRenewalAvailable = false;
    std::string adHeader;
    std::string adDescription;
    std::string adEndpoint;
};

// Соответствует amnezia::ApiConfig (модель ответа account_info / api_config сервера).
struct ApiConfig {
    std::string serviceType;
    std::string serviceProtocol;
    std::string userCountryCode;
    std::string serverCountryCode;
    std::string serverCountryName;
    std::string vpnKey;

    std::string subscriptionEndDate;  // subscription.end_date

    int activeDeviceCount = 0;
    int maxDeviceCount = 0;
    int issuedConfigs = 0;

    // Массивы оставляем как сырой JSON (строка) — детальные UI-модели строит приложение.
    std::string availableCountriesJson;  // available_countries (JSON-массив)
    std::string supportedProtocolsJson;  // supported_protocols (JSON-массив)

    ServiceInfo serviceInfo;
    std::string publicKeyExpiresAt;  // public_key.expires_at

    std::string stackType;
    std::string cliVersion;
    bool isTestPurchase = false;
    bool isInAppPurchase = false;
    bool subscriptionExpiredByServer = false;
};

} // namespace agw

#endif // AGW_MODELS_H
