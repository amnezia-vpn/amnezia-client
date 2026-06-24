#ifndef AGW_CRYPTO_AES_H
#define AGW_CRYPTO_AES_H

#include <cstdint>
#include <vector>

namespace agw::crypto {

// AES-256-CBC, паддинг PKCS7 (дефолт EVP) — паритет с QSimpleCrypto::QBlockCipher.
//
// Контракт по паритету:
//  - key: ровно 32 байта (AES-256);
//  - iv: >= 16 байт; CBC использует ТОЛЬКО первые 16 (оригинал генерит 32, берёт 16);
//  - salt в локальном AES не участвует (ветка EVP_BytesToKey отключена) — здесь его просто нет.
//
// Бросают std::runtime_error при ошибке OpenSSL (decrypt — в т.ч. при неверном ключе/паддинге).
std::vector<std::uint8_t> aesEncryptCbc(const std::vector<std::uint8_t> &data,
                                        const std::vector<std::uint8_t> &key,
                                        const std::vector<std::uint8_t> &iv);

std::vector<std::uint8_t> aesDecryptCbc(const std::vector<std::uint8_t> &data,
                                        const std::vector<std::uint8_t> &key,
                                        const std::vector<std::uint8_t> &iv);

} // namespace agw::crypto

#endif // AGW_CRYPTO_AES_H
