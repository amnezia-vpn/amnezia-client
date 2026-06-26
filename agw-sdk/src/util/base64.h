#ifndef AGW_UTIL_BASE64_H
#define AGW_UTIL_BASE64_H

#include <cstdint>
#include <string>
#include <vector>

namespace agw::util
{

    // Стандартный base64 (алфавит +/, с паддингом '=') — как QByteArray::toBase64() по умолчанию.
    // Используется для key_payload/api_payload в теле запроса.
    std::string base64Encode(const std::vector<std::uint8_t> &data);

    // base64url (алфавит -_) без хвостовых '=' — как
    // QByteArray::toBase64(Base64UrlEncoding | OmitTrailingEquals). Используется только для путей S3.
    std::string base64UrlEncodeNoPad(const std::vector<std::uint8_t> &data);

    // Декодирует и стандартный, и url-алфавит; паддинг опционален. Невалидные символы пропускаются
    // (как Qt fromBase64 по умолчанию). Используется для разбора ответа/S3-списка.
    std::vector<std::uint8_t> base64Decode(const std::string &text);

    // Перегрузки для строк (тело — это std::string).
    std::string base64Encode(const std::string &data);

} // namespace agw::util

#endif // AGW_UTIL_BASE64_H
