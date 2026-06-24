#ifndef AGW_CRYPTO_HASH_H
#define AGW_CRYPTO_HASH_H

#include <cstdint>
#include <string>
#include <vector>

namespace agw::crypto {

// SHA-512: 64 байта дайджеста.
std::vector<std::uint8_t> sha512(const std::vector<std::uint8_t> &data);

// Lowercase hex (как QByteArray::toHex()).
std::string toHex(const std::vector<std::uint8_t> &data);

// fromHex (как QByteArray::fromHex()). Бросает при нечётной длине/неверном символе.
std::vector<std::uint8_t> fromHex(const std::string &hex);

} // namespace agw::crypto

#endif // AGW_CRYPTO_HASH_H
