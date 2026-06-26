#ifndef AGW_PROTOCOL_KEYS_H
#define AGW_PROTOCOL_KEYS_H

namespace agw::protocol::keys {
inline constexpr const char *aesKey = "aes_key";
inline constexpr const char *aesIv = "aes_iv";
inline constexpr const char *aesSalt = "aes_salt";
inline constexpr const char *apiPayload = "api_payload";
inline constexpr const char *keyPayload = "key_payload";

inline constexpr const char *serviceType = "service_type";
inline constexpr const char *userCountryCode = "user_country_code";

inline constexpr const char *httpStatus = "http_status";
inline constexpr const char *message = "message";

// Поля для сборки vpn-ключа и разбора конфигов (Tier 2).
inline constexpr const char *name = "name";
inline constexpr const char *description = "description";
inline constexpr const char *configVersion = "config_version";
inline constexpr const char *protocol = "protocol";
inline constexpr const char *apiEndpoint = "api_endpoint";
inline constexpr const char *apiKey = "api_key";
inline constexpr const char *apiConfig = "api_config";
inline constexpr const char *serviceProtocol = "service_protocol";
inline constexpr const char *authData = "auth_data";
inline constexpr const char *subscriptionEndDate = "end_date";

// Поля payload типизированных методов (Tier 2, Шаг 1).
inline constexpr const char *osVersion = "os_version";
inline constexpr const char *appVersion = "app_version";
inline constexpr const char *cliName = "cli_name";
inline constexpr const char *cliVersion = "cli_version";
inline constexpr const char *appLanguage = "app_language";
inline constexpr const char *installationUuid = "installation_uuid";
inline constexpr const char *services = "services";
inline constexpr const char *locale = "locale";
inline constexpr const char *news = "news";
inline constexpr const char *url = "url";

// account_info / renewal_link (Шаг 2).
inline constexpr const char *serverCountryCode = "server_country_code";
inline constexpr const char *subscriptionStatus = "subscription_status";
inline constexpr const char *renewalUrl = "renewal_url";

// import/trial/captcha/deactivate (Шаг 3).
inline constexpr const char *publicKey = "public_key";
inline constexpr const char *email = "email";
inline constexpr const char *captchaId = "captcha_id";
inline constexpr const char *captchaSolution = "captcha_solution";
inline constexpr const char *captchaImage = "captcha_image";
inline constexpr const char *hint = "hint";
inline constexpr const char *config = "config";
inline constexpr const char *protoAwg = "awg";
inline constexpr const char *protoVless = "vless";
}

#endif
