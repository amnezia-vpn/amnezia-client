#ifndef AGW_PROTOCOL_REQUEST_BUILDER_H
#define AGW_PROTOCOL_REQUEST_BUILDER_H

#include <cstdint>
#include <string>
#include <vector>

#include "agw/types.h"
#include "crypto/rng.h"

namespace agw::protocol {

// Зашифрованное тело запроса + ключи для расшифровки ответа.
struct EncryptedRequest {
    std::string body;               // сериализованное { api_payload, key_payload } (Qt-Indented)
    std::vector<std::uint8_t> key;  // AES-ключ (для расшифровки ответа)
    std::vector<std::uint8_t> iv;   // AES-IV (CBC берёт первые 16)
    std::vector<std::uint8_t> salt; // 8 байт; в локальном AES не участвует
    ErrorCode error = ErrorCode::NoError;
};

// Собирает тело по протоколу (паритет с GatewayController::prepareRequest, крипто-часть):
//  - key/iv/salt из rng (32/32/8);
//  - key_payload = base64(RSA_PKCS1(json_keys)); api_payload = base64(AES-256-CBC(payload));
//  - body = QtIndented({ key_payload, api_payload }).
// Ошибки как в оригинале: невалидный публичный ключ → ApiMissingAgwPublicKey;
// сбой шифрования → ApiConfigDecryptionError.
EncryptedRequest buildEncryptedRequest(const std::string &payload,
                                       const std::string &publicKeyPem,
                                       crypto::IRng &rng);

} // namespace agw::protocol

#endif // AGW_PROTOCOL_REQUEST_BUILDER_H
