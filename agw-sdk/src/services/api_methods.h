#ifndef AGW_SERVICES_API_METHODS_H
#define AGW_SERVICES_API_METHODS_H

#include <cstdint>
#include <string>
#include <vector>

#include "agw/gateway_controller.h"
#include "agw/models.h"
#include "agw/types.h"
#include "util/json.h"

// Типизированные read-методы Tier 2 (Шаг 1) поверх транспорта GatewayController::post.
// Внутренний слой (использует nlohmann внутри); публичная/C-ABI обёртка — позже.
namespace agw::services {

struct ServicesRequest {
    std::string osVersion;
    std::string appVersion;
    std::string cliName;
    std::string appLanguage;  // "en"
};

struct NewsRequest {
    std::string locale;                            // "en"
    std::vector<std::string> userCountryCodes;     // из серверов приложения (параметр, не из настроек)
    std::vector<std::string> serviceTypes;
};

struct UpdaterRequest {
    std::string cliVersion;
    std::string osVersion;
    std::string installationUuid;
};

struct JsonResult {
    ErrorCode error = ErrorCode::NoError;
    util::Json value;  // services: объект ответа; news: массив новостей
};

struct UpdateResult {
    ErrorCode error = ErrorCode::NoError;
    std::string url;   // base_url без хвостового '/'
    util::Json raw;
};

// v1/services: payload {os_version, app_version, cli_name, app_language}; в ответе ожидается "services".
JsonResult getServices(GatewayController &gw, const ServicesRequest &req);

// v1/news: payload {locale, [user_country_code], [service_type]}; ответ — массив или {news:[...]}.
JsonResult getNews(GatewayController &gw, const NewsRequest &req);

// v1/updater_endpoint: payload {cli_version, os_version, installation_uuid}; ответ {url:...}.
UpdateResult getUpdaterEndpoint(GatewayController &gw, const UpdaterRequest &req);

// Базовый запрос gateway (паритет с GatewayRequestData::toJsonObject — кладёт только непустые поля).
// Входы приходят из приложения (не из настроек). authData — JSON-объект (обычно {api_key: ...}).
struct GatewayRequest {
    std::string osVersion;
    std::string appVersion;
    std::string appLanguage;
    std::string installationUuid;
    std::string userCountryCode;
    std::string serverCountryCode;
    std::string serviceType;
    std::string serviceProtocol;
    util::Json authData;  // опционально; если объект непустой — кладётся в auth_data
};

// account_info / renewal_link используют один payload: GatewayRequest + cli_version + subscription_status.
struct AccountRequest {
    GatewayRequest base;
    std::string cliVersion;
    std::string subscriptionStatus;
};

struct AccountInfoResult {
    ErrorCode error = ErrorCode::NoError;
    ApiConfig account;
};

struct RenewalResult {
    ErrorCode error = ErrorCode::NoError;
    std::string renewalUrl;
};

// v1/account_info → распарсенный ApiConfig.
AccountInfoResult getAccountInfo(GatewayController &gw, const AccountRequest &req);

// v1/renewal_link → renewal_url.
RenewalResult getRenewalLink(GatewayController &gw, const AccountRequest &req);

// --- Шаг 3: мутации подписки + капча ----------------------------------------

struct CaptchaInfo {
    std::string captchaId;
    std::string captchaImageBase64;  // картинку рисует приложение (UI), не SDK
    std::string hint;
};

// Результат import/trial/resolveCaptcha. Если captchaRequired — приложение показывает картинку и
// зовёт resolveImportCaptcha. На успехе serverConfigJson — РАСПАКОВАННЫЙ конфиг с плейсхолдером
// $WIREGUARD_CLIENT_PRIVATE_KEY: подстановку приватного ключа и персист сервера делает приложение.
struct ImportResult {
    ErrorCode error = ErrorCode::NoError;
    bool captchaRequired = false;
    CaptchaInfo captcha;
    std::string serverConfigJson;
    std::string rawResponseJson;  // сырое расшифрованное тело ответа (для service_info/supported_protocols в app)
};

// public_key в payload: для awg — WireGuard client pub key, для vless — xray uuid (определяется
// по req.serviceProtocol). Генерация ключей — в приложении; сюда приходит готовое значение.
ImportResult importService(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey);

ImportResult importTrial(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                         const std::string &email);

ImportResult resolveImportCaptcha(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                                  const std::string &captchaId, const std::string &captchaSolution);

// v1/revoke_config. Приложение трактует ApiNotFoundError как успех и само правит/удаляет сервер.
ErrorCode deactivateDevice(GatewayController &gw, const GatewayRequest &req);

// --- Шаг 4: iOS IAP (App Store) — шлюзовая часть ----------------------------

// Результат v1/subscriptions. SDK делает только сетевую/крипто-часть: payload + POST + разбор ответа
// (поле "key" = vpn://...), распаковку конфига и crc (qChecksum/CRC-16 над Qt-indented JSON конфига).
// vpnKey — нормализованный ключ без префикса "vpn://" (приложение хранит его и дедуплицирует серверы).
// StoreKit (покупка/восстановление/цены/транзакции), флаги isInAppPurchase/isTestPurchase, дедуп и
// персист сервера — в приложении. Test/Sandbox vs Prod выбирается КОНФИГОМ переданного GatewayController.
struct AppStoreImportResult {
    ErrorCode error = ErrorCode::NoError;
    std::string serverConfigJson;  // распакованный конфиг ($WIREGUARD_CLIENT_PRIVATE_KEY не трогаем)
    std::string vpnKey;            // normalizedKey (base64url без "vpn://")
    std::uint16_t crc = 0;         // qChecksum(qtIndentedDump(config)) — приложение кладёт в ApiV2ServerConfig
};

// v1/subscriptions: payload = GatewayRequest + public_key (по протоколу) + transaction_id.
// transactionId приходит из StoreKit (приложение). На успехе config_version обязан быть AmneziaGateway(2).
AppStoreImportResult importServiceFromAppStore(GatewayController &gw, const GatewayRequest &req,
                                               const std::string &publicKey, const std::string &transactionId);

// --- Фаза B: update существующего сервиса + native_config + raw account_info -----------------

// v1/config для существующего сервиса (authData в req). isConnectEvent → is_connect_event=true.
ImportResult updateService(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                           bool isConnectEvent);

// v1/account_info → сырое разобранное тело (приложение само читает нужные поля).
JsonResult getAccountInfoRaw(GatewayController &gw, const AccountRequest &req);

struct NativeConfigResult {
    ErrorCode error = ErrorCode::NoError;
    std::string config;  // поле config из ответа (открытый текст; $WIREGUARD_CLIENT_PRIVATE_KEY цел)
};

// v1/native_config → config (нативный конфиг текстом). Подстановку WG-ключа делает приложение.
NativeConfigResult exportNativeConfig(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey);

// v1/revoke_native_config. ApiNotFoundError приложение трактует как успех.
ErrorCode revokeNativeConfig(GatewayController &gw, const GatewayRequest &req);

} // namespace agw::services

#endif // AGW_SERVICES_API_METHODS_H
