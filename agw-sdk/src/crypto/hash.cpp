#include "hash.h"

#include <stdexcept>

#include <openssl/sha.h>

namespace agw::crypto
{

    namespace
    {

        int hexNibble(char c)
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        }

    } // namespace

    std::vector<std::uint8_t> sha512(const std::vector<std::uint8_t> &data)
    {
        std::vector<std::uint8_t> out(SHA512_DIGEST_LENGTH);
        SHA512(data.data(), data.size(), out.data());
        return out;
    }

    std::string toHex(const std::vector<std::uint8_t> &data)
    {
        static const char *digits = "0123456789abcdef";
        std::string out;
        out.reserve(data.size() * 2);
        for (std::uint8_t b : data) {
            out.push_back(digits[b >> 4]);
            out.push_back(digits[b & 0x0F]);
        }
        return out;
    }

    std::vector<std::uint8_t> fromHex(const std::string &hex)
    {
        if (hex.size() % 2 != 0) {
            throw std::runtime_error("agw::crypto::fromHex: odd-length input");
        }
        std::vector<std::uint8_t> out;
        out.reserve(hex.size() / 2);
        for (std::size_t i = 0; i < hex.size(); i += 2) {
            const int hi = hexNibble(hex[i]);
            const int lo = hexNibble(hex[i + 1]);
            if (hi < 0 || lo < 0) {
                throw std::runtime_error("agw::crypto::fromHex: invalid hex character");
            }
            out.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
        }
        return out;
    }

} // namespace agw::crypto
