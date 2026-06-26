#ifndef AGW_CRYPTO_RSA_H
#define AGW_CRYPTO_RSA_H

#include <cstdint>
#include <string>
#include <vector>

namespace agw::crypto
{

    // RSA-шифрование публичным ключом, паддинг RSA_PKCS1_PADDING (v1.5) — паритет с QSimpleCrypto::QRsa.
    // Ключ из PEM (PEM_read_bio_PUBKEY). Паддинг недетерминирован (рандом из глобального RNG OpenSSL),
    // поэтому шифротекст НЕ воспроизводим байт-в-байт — golden проверяет round-trip, не байты.
    // Бросает std::runtime_error при ошибке загрузки ключа или шифрования.
    std::vector<std::uint8_t> rsaEncryptPublicPkcs1(const std::vector<std::uint8_t> &plaintext,
                                                    const std::string &publicKeyPem);

    // Расшифровка приватным ключом (PKCS1 v1.5). Нужна тестам (round-trip) и заделу под dev-инструменты.
    std::vector<std::uint8_t> rsaDecryptPrivatePkcs1(const std::vector<std::uint8_t> &ciphertext,
                                                     const std::string &privateKeyPem);

    // Загружается ли PEM как публичный ключ. Нужно для маппинга ошибок: невалидный/отсутствующий
    // ключ → ApiMissingAgwPublicKey (как в оригинале, отдельной веткой до шифрования).
    bool rsaPublicKeyValid(const std::string &publicKeyPem);

} // namespace agw::crypto

#endif // AGW_CRYPTO_RSA_H
