#include "uuid.h"

#include <cstdint>
#include <vector>

namespace agw::util {

namespace {

const char *kHex = "0123456789abcdef";

void appendHex(std::string &out, std::uint8_t b)
{
    out.push_back(kHex[b >> 4]);
    out.push_back(kHex[b & 0x0F]);
}

} // namespace

std::string makeUuidV4(crypto::IRng &rng)
{
    std::vector<std::uint8_t> b = rng.bytes(16);

    // версия 4 в старшем полубайте 7-го байта; вариант RFC 4122 (10xxxxxx) в 9-м байте.
    b[6] = static_cast<std::uint8_t>((b[6] & 0x0F) | 0x40);
    b[8] = static_cast<std::uint8_t>((b[8] & 0x3F) | 0x80);

    std::string out;
    out.reserve(36);
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            out.push_back('-');
        }
        appendHex(out, b[i]);
    }
    return out;
}

} // namespace agw::util
