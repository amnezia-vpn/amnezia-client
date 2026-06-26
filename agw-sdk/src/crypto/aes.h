#ifndef AGW_CRYPTO_AES_H
#define AGW_CRYPTO_AES_H

#include <cstdint>
#include <vector>

namespace agw::crypto
{
    std::vector<std::uint8_t> aesEncryptCbc(const std::vector<std::uint8_t> &data, const std::vector<std::uint8_t> &key,
                                            const std::vector<std::uint8_t> &iv);

    std::vector<std::uint8_t> aesDecryptCbc(const std::vector<std::uint8_t> &data, const std::vector<std::uint8_t> &key,
                                            const std::vector<std::uint8_t> &iv);
}

#endif
