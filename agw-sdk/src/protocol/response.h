#ifndef AGW_PROTOCOL_RESPONSE_H
#define AGW_PROTOCOL_RESPONSE_H

#include <cstdint>
#include <string>
#include <vector>

namespace agw::protocol {

struct DecryptResult {
    std::string decryptedBody;     // расшифрованное тело; при провале — исходные (сырые) байты
    bool ok = false;
};

// Расшифровка тела ответа AES-256-CBC тем же key/iv, что в запросе (salt не используется).
// Тело — сырые байты ответа (НЕ base64). При исключении: ok=false, decryptedBody=encrypted
// (паритет с GatewayController::tryDecryptResponseBody).
DecryptResult tryDecryptResponse(const std::string &encrypted,
                                 const std::vector<std::uint8_t> &key,
                                 const std::vector<std::uint8_t> &iv);

} // namespace agw::protocol

#endif // AGW_PROTOCOL_RESPONSE_H
