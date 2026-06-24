#ifndef AGW_PROTOCOL_KEYS_H
#define AGW_PROTOCOL_KEYS_H

// Имена полей API-протокола (Qt-free перенос нужного подмножества client/.../apiKeys.h).
// Только то, что нужно транспорту v1: тело запроса и контекст failover.
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

} // namespace agw::protocol::keys

#endif // AGW_PROTOCOL_KEYS_H
