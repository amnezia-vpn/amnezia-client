#ifndef AGW_CRYPTO_RNG_H
#define AGW_CRYPTO_RNG_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace agw::crypto
{

    // Источник случайных байт. Инъектируется ради детерминированных golden-тестов.
    // Замечание: RSA PKCS#1 v1.5 берёт паддинг из глобального RNG OpenSSL, не отсюда,
    // поэтому key_payload недетерминирован даже при фиксированном IRng (см. план, golden-стратегия).
    class IRng
    {
    public:
        virtual ~IRng() = default;
        virtual std::vector<std::uint8_t> bytes(std::size_t n) = 0;
    };

    // Боевой RNG: RAND_priv_bytes (как QBlockCipher::generatePrivateSalt в оригинале).
    class DefaultRng : public IRng
    {
    public:
        std::vector<std::uint8_t> bytes(std::size_t n) override;
    };

} // namespace agw::crypto

#endif // AGW_CRYPTO_RNG_H
