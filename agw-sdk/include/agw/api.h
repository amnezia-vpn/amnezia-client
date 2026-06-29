#ifndef AGW_API_H
#define AGW_API_H

#include <cstdint>
#include <string>
#include <vector>

#include "agw/models.h"
#include "agw/types.h"

// Публичный C++-слой Tier 2: типизированные методы Amnezia API поверх транспорта GatewayController.
// Только std-типы — ни nlohmann, ни Qt наружу не течёт (внутри делегирует в src/services/*).
// Это бридж для C++-потребителей (клиент линкует agw::agw и видит только include/agw/).
namespace agw {

class GatewayController;

namespace api {

// Базовый gateway-запрос (паритет с GatewayRequestData::toJsonObject — пустые поля опускаются).
// authDataJson — опциональный JSON-объект текстом (напр. {"api_key":"..."}); пусто — пропустить.
struct GatewayRequest {
    std::string osVersion;
    std::string appVersion;
    std::string appLanguage;
    std::string installationUuid;
    std::string userCountryCode;
    std::string serverCountryCode;
    std::string serviceType;
    std::string serviceProtocol;
    std::string authDataJson;
};

// Результат с одним JSON-телом текстом (services — объект; news — массив).
struct JsonResult {
    ErrorCode error = ErrorCode::NoError;
    std::string json;
};

struct UrlResult {
    ErrorCode error = ErrorCode::NoError;
    std::string url;
    std::string rawJson;  // сырой ответ (для updater_endpoint); может быть пустым
};

struct AccountInfoResult {
    ErrorCode error = ErrorCode::NoError;
    ApiConfig account;
};

struct RenewalResult {
    ErrorCode error = ErrorCode::NoError;
    std::string renewalUrl;
};

struct CaptchaInfo {
    std::string captchaId;
    std::string captchaImageBase64;  // картинку рисует приложение
    std::string hint;
};

// На успехе serverConfigJson — РАСПАКОВАННЫЙ конфиг с плейсхолдером $WIREGUARD_CLIENT_PRIVATE_KEY:
// подстановку ключа, миграцию контейнеров и персист сервера делает приложение.
struct ImportResult {
    ErrorCode error = ErrorCode::NoError;
    bool captchaRequired = false;
    CaptchaInfo captcha;
    std::string serverConfigJson;
};

// v1/subscriptions (App Store). vpnKey — без префикса "vpn://"; crc — qChecksum/CRC-16.
struct AppStoreImportResult {
    ErrorCode error = ErrorCode::NoError;
    std::string serverConfigJson;
    std::string vpnKey;
    std::uint16_t crc = 0;
};

JsonResult getServices(GatewayController &gw, const std::string &osVersion, const std::string &appVersion,
                       const std::string &cliName, const std::string &appLanguage);

JsonResult getNews(GatewayController &gw, const std::string &locale,
                   const std::vector<std::string> &userCountryCodes, const std::vector<std::string> &serviceTypes);

UrlResult getUpdaterEndpoint(GatewayController &gw, const std::string &cliVersion, const std::string &osVersion,
                             const std::string &installationUuid);

AccountInfoResult getAccountInfo(GatewayController &gw, const GatewayRequest &req, const std::string &cliVersion,
                                 const std::string &subscriptionStatus);

RenewalResult getRenewalLink(GatewayController &gw, const GatewayRequest &req, const std::string &cliVersion,
                             const std::string &subscriptionStatus);

ImportResult importService(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey);

ImportResult importTrial(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                         const std::string &email);

ImportResult resolveImportCaptcha(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                                  const std::string &captchaId, const std::string &captchaSolution);

ErrorCode deactivateDevice(GatewayController &gw, const GatewayRequest &req);

AppStoreImportResult importServiceFromAppStore(GatewayController &gw, const GatewayRequest &req,
                                               const std::string &publicKey, const std::string &transactionId);

} // namespace api
} // namespace agw

#endif // AGW_API_H
