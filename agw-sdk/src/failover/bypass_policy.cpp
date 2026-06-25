#include "failover/bypass_policy.h"

#include "protocol/keys.h"
#include "util/json.h"

namespace agw::failover {

namespace {

// Строки реальных ошибок — при них НЕ байпасим (это валидный ответ, а не блокировка).
constexpr const char *kPattern1 = "No active configuration found for";
constexpr const char *kPattern2 = "No non-revoked public key found for";
constexpr const char *kPattern3 = "Account not found.";
constexpr const char *kPatternQrSessionNotFound = "QR session not found";
constexpr const char *kPatternSessionNotFound = "Session not found";
constexpr const char *kUpdateRequestPattern = "client version update is required";
constexpr const char *kUnprocessableSubscriptionMessage =
    "Failed to retrieve subscription information. Is it activated?";

constexpr int kNotFound = 404;
constexpr int kNotImplemented = 501;
constexpr int kPaymentRequired = 402;
constexpr int kConflict = 409;
constexpr int kRequestTimeout = 408;
constexpr int kUnprocessableEntity = 422;

bool contains(const std::string &body, const char *needle)
{
    return body.find(needle) != std::string::npos;
}

std::string trim(const std::string &s)
{
    std::size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\n' || s[b] == '\r')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\n' || s[e - 1] == '\r')) --e;
    return s.substr(b, e - b);
}

} // namespace

bool shouldBypassProxy(TransportError transportError, const std::string &decryptedBody, bool decryptionSuccessful)
{
    if (!decryptionSuccessful) {
        return true;
    }

    int apiHttpStatus = -1;
    std::string apiErrorMessage;
    try {
        util::Json obj = util::Json::parse(decryptedBody);
        if (obj.is_object()) {
            if (auto it = obj.find(protocol::keys::httpStatus); it != obj.end() && it->is_number_integer()) {
                apiHttpStatus = it->get<int>();
            }
            if (auto it = obj.find(protocol::keys::message); it != obj.end() && it->is_string()) {
                apiErrorMessage = trim(it->get<std::string>());
            }
        }
    } catch (...) {
        // не объект — apiHttpStatus остаётся -1
    }

    if (transportError == TransportError::Canceled || transportError == TransportError::Timeout) {
        return true;
    }
    if (contains(decryptedBody, "html")) {
        return true;
    }
    if (apiHttpStatus == kRequestTimeout) {
        return false;
    }
    if (apiHttpStatus == kNotFound) {
        if (contains(decryptedBody, kPattern1) || contains(decryptedBody, kPattern2)
            || contains(decryptedBody, kPattern3) || contains(decryptedBody, kPatternQrSessionNotFound)
            || contains(decryptedBody, kPatternSessionNotFound)) {
            return false;
        }
        return true;
    }
    if (apiHttpStatus == kNotImplemented) {
        if (contains(decryptedBody, kUpdateRequestPattern)) {
            return false;
        }
        return true;
    }
    if (apiHttpStatus == kConflict) {
        return false;
    }
    if (apiHttpStatus == kPaymentRequired) {
        return false;
    }
    if (apiHttpStatus == kUnprocessableEntity) {
        return apiErrorMessage != kUnprocessableSubscriptionMessage;
    }
    if (transportError != TransportError::None) {
        return true;
    }
    return false;
}

} // namespace agw::failover
