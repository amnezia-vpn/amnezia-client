#ifndef AGW_UTIL_ZLIB_H
#define AGW_UTIL_ZLIB_H

#include <cstdint>
#include <vector>

namespace agw::util {

// Паритет с Qt qCompress/qUncompress: формат = 4-байтовый префикс длины (big-endian, размер
// исходных данных) + zlib-поток (как zlib compress2). Нужно для сборки vpn-ключа и разбора конфигов.
std::vector<std::uint8_t> qtCompress(const std::vector<std::uint8_t> &data, int level);

// Разбирает Qt-формат: читает 4-байтовый префикс длины, распаковывает остаток.
// Бросает std::runtime_error при повреждении (как qUncompress вернул бы пусто — но мы сигналим).
std::vector<std::uint8_t> qtUncompress(const std::vector<std::uint8_t> &data);

} // namespace agw::util

#endif // AGW_UTIL_ZLIB_H
