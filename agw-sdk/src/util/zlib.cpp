#include "util/zlib.h"

#include <stdexcept>

#include <zlib.h>

namespace agw::util {

std::vector<std::uint8_t> qtCompress(const std::vector<std::uint8_t> &data, int level)
{
    uLongf bound = compressBound(static_cast<uLong>(data.size()));
    std::vector<std::uint8_t> out(4 + bound);

    // 4-байтовый префикс: размер исходных данных, big-endian (как Qt qCompress).
    const std::uint32_t n = static_cast<std::uint32_t>(data.size());
    out[0] = static_cast<std::uint8_t>((n >> 24) & 0xFF);
    out[1] = static_cast<std::uint8_t>((n >> 16) & 0xFF);
    out[2] = static_cast<std::uint8_t>((n >> 8) & 0xFF);
    out[3] = static_cast<std::uint8_t>(n & 0xFF);

    uLongf destLen = bound;
    const int rc = compress2(out.data() + 4, &destLen,
                             data.data(), static_cast<uLong>(data.size()), level);
    if (rc != Z_OK) {
        throw std::runtime_error("agw::util::qtCompress: compress2 failed");
    }
    out.resize(4 + destLen);
    return out;
}

std::vector<std::uint8_t> qtUncompress(const std::vector<std::uint8_t> &data)
{
    if (data.size() < 4) {
        throw std::runtime_error("agw::util::qtUncompress: input shorter than 4-byte header");
    }
    const std::uint32_t expected = (static_cast<std::uint32_t>(data[0]) << 24)
                                 | (static_cast<std::uint32_t>(data[1]) << 16)
                                 | (static_cast<std::uint32_t>(data[2]) << 8)
                                 | static_cast<std::uint32_t>(data[3]);

    std::vector<std::uint8_t> out(expected);
    uLongf destLen = expected;
    const int rc = uncompress(out.data(), &destLen, data.data() + 4,
                              static_cast<uLong>(data.size() - 4));
    if (rc != Z_OK) {
        throw std::runtime_error("agw::util::qtUncompress: uncompress failed");
    }
    out.resize(destLen);
    return out;
}

} // namespace agw::util
