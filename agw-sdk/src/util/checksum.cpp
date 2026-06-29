#include "util/checksum.h"

namespace agw::util {

namespace {

// Рефлектированная таблица для полинома 0x1021 (как в Qt qChecksum), обработка по нибблам.
constexpr std::uint16_t kCrcTbl[16] = {
    0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b, 0xc60c, 0xd68d, 0xe70e, 0xf78f,
};

} // namespace

std::uint16_t qtChecksum16(const std::uint8_t *data, std::size_t len)
{
    std::uint16_t crc = 0xffff;
    for (std::size_t i = 0; i < len; ++i) {
        std::uint8_t c = data[i];
        crc = ((crc >> 4) & 0x0fff) ^ kCrcTbl[(crc ^ c) & 0xf];
        c = static_cast<std::uint8_t>(c >> 4);
        crc = ((crc >> 4) & 0x0fff) ^ kCrcTbl[(crc ^ c) & 0xf];
    }
    return static_cast<std::uint16_t>(~crc & 0xffff);
}

std::uint16_t qtChecksum16(const std::string &data)
{
    return qtChecksum16(reinterpret_cast<const std::uint8_t *>(data.data()), data.size());
}

} // namespace agw::util
