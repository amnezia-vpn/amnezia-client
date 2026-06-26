#ifndef AGW_CRYPTO_HASH_H
#define AGW_CRYPTO_HASH_H

#include <cstdint>
#include <string>
#include <vector>

namespace agw::crypto
{
    std::vector<std::uint8_t> sha512(const std::vector<std::uint8_t> &data);

    std::string toHex(const std::vector<std::uint8_t> &data);

    std::vector<std::uint8_t> fromHex(const std::string &hex);
}

#endif
