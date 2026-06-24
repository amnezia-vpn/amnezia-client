#include "protocol/error_mapping.h"

#include <algorithm>
#include <cctype>

#include "protocol/keys.h"
#include "util/json.h"

namespace agw::protocol {

namespace {

// http_status, влияющие на маппинг (как в apiUtils.cpp).
constexpr int kConflict = 409;
constexpr int kNotFound = 404;
constexpr int kNotImplemented = 501;
constexpr int kPaymentRequired = 402;
constexpr int kTooManyRequests = 429;
constexpr int kRequestTimeout = 408;
constexpr int kUnprocessableEntity = 422;

constexpr const char *kUnprocessableSubscriptionMessage =
    "Failed to retrieve subscription information. Is it activated?";
constexpr const char *kTrialAlreadyUsedMessage = "trial subscription already used";

std::string trim(const std::string &s)
{
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool containsCI(const std::string &haystack, const std::string &needle)
{
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

std::string messageFrom(const util::Json &obj)
{
    auto it = obj.find(keys::message);
    if (it != obj.end() && it->is_string()) {
        return trim(it->get<std::string>());
    }
    return {};
}

} // namespace

ErrorCode mapResponseError(bool sslError, TransportError transportError, const std::string &decryptedBody)
{
    if (sslError) {
        return ErrorCode::ApiConfigSslError;
    }
    if (transportError == TransportError::Timeout || transportError == TransportError::Canceled) {
        return ErrorCode::ApiConfigTimeoutError;
    }
    if (transportError == TransportError::OperationNotImplemented) {
        return ErrorCode::ApiUpdateRequestError;
    }

    util::Json obj;
    bool isObject = false;
    try {
        obj = util::Json::parse(decryptedBody);
        isObject = obj.is_object();
    } catch (...) {
        isObject = false;
    }

    if (isObject) {
        int httpStatus = -1;
        if (auto it = obj.find(keys::httpStatus); it != obj.end() && it->is_number_integer()) {
            httpStatus = it->get<int>();
        }
        const std::string message = messageFrom(obj);

        if (httpStatus == kTooManyRequests) {
            return ErrorCode::ApiRateLimitError;
        }
        if (httpStatus == kConflict) {
            if (containsCI(message, kTrialAlreadyUsedMessage)) {
                return ErrorCode::ApiTrialAlreadyUsedError;
            }
            return ErrorCode::ApiConfigLimitError;
        }
        if (httpStatus == kNotFound) {
            return ErrorCode::ApiNotFoundError;
        }
        if (httpStatus == kRequestTimeout) {
            return ErrorCode::ApiConfigTimeoutError;
        }
        if (httpStatus == kNotImplemented) {
            return ErrorCode::ApiUpdateRequestError;
        }
        if (httpStatus == kUnprocessableEntity) {
            if (message == kUnprocessableSubscriptionMessage) {
                return ErrorCode::ApiSubscriptionExpiredError;
            }
            return ErrorCode::ApiConfigDownloadError;
        }
        if (httpStatus == kPaymentRequired) {
            if (containsCI(message, "refresh_captcha")) {
                return ErrorCode::ApiCaptchaRefreshError;
            }
            if (containsCI(message, "invalid_captcha")) {
                return ErrorCode::ApiCaptchaInvalidError;
            }
            if (obj.contains("captcha_id") || obj.contains("captcha_image")
                || containsCI(message, "rate_limit_exceeded")) {
                return ErrorCode::ApiCaptchaRequiredError;
            }
            return ErrorCode::ApiSubscriptionNotActiveError;
        }
        if (httpStatus >= 300) {
            return ErrorCode::ApiConfigDownloadError;
        }
    }

    if (transportError == TransportError::None) {
        return ErrorCode::NoError;
    }
    return ErrorCode::ApiConfigDownloadError;
}

} // namespace agw::protocol
