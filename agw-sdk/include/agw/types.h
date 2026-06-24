#ifndef AGW_TYPES_H
#define AGW_TYPES_H

#include <string>

namespace agw {

// Подмножество кодов API-шлюза (1100–1120 из client/core/utils/errorCodes.h).
// Numeric-значения сохранены один в один, чтобы Qt-адаптер мапил их без таблицы.
// NoError и Cancelled — служебные значения SDK (Cancelled — best-effort отмена, Фаза 4).
enum class ErrorCode : int {
    NoError = 0,
    Cancelled = 1,

    ApiConfigDownloadError = 1100,
    ApiConfigAlreadyAdded = 1101,
    ApiConfigEmptyError = 1102,
    ApiConfigTimeoutError = 1103,
    ApiConfigSslError = 1104,
    ApiMissingAgwPublicKey = 1105,
    ApiConfigDecryptionError = 1106,
    ApiServicesMissingError = 1107,
    ApiConfigLimitError = 1108,
    ApiNotFoundError = 1109,
    ApiMigrationError = 1110,
    ApiUpdateRequestError = 1111,
    ApiSubscriptionExpiredError = 1112,
    ApiPurchaseError = 1113,
    ApiSubscriptionNotActiveError = 1114,
    ApiNoPurchasedSubscriptionsError = 1115,
    ApiTrialAlreadyUsedError = 1116,
    ApiCaptchaRequiredError = 1117,
    ApiCaptchaInvalidError = 1118,
    ApiCaptchaRefreshError = 1119,
    ApiRateLimitError = 1120,
};

enum class LogLevel : int { Debug, Info, Warning, Error };

// Результат post(): расшифрованное тело (или тело ошибки) + код.
// body несётся наружу даже при ненулевом error — приложение читает тело ошибки.
struct Response {
    ErrorCode error = ErrorCode::NoError;
    std::string body;
};

// Контекст failover: по этим полям строятся пути S3 (endpoints-<service>-<country>.json).
// Передаётся отдельно от payload — SDK не парсит чужой payload.
struct FailoverContext {
    std::string serviceType;      // service_type
    std::string userCountryCode;  // user_country_code
};

} // namespace agw

#endif // AGW_TYPES_H
