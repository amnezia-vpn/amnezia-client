#ifndef AGW_UTIL_CHECKSUM_H
#define AGW_UTIL_CHECKSUM_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace agw::util {

// Паритет с Qt qChecksum (стандарт по умолчанию — Qt::ChecksumIso3309): CRC-16/X-25,
// полином 0x1021 (рефлектированный 0x8408), init 0xFFFF, refin/refout, xorout 0xFFFF.
// Нужно для поля crc в ApiV2ServerConfig (iOS IAP). Считается над Qt-indented JSON конфига.
std::uint16_t qtChecksum16(const std::uint8_t *data, std::size_t len);

std::uint16_t qtChecksum16(const std::string &data);

} // namespace agw::util

#endif // AGW_UTIL_CHECKSUM_H
