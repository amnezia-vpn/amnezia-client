#ifndef AGW_UTIL_UUID_H
#define AGW_UTIL_UUID_H

#include <string>

#include "crypto/rng.h"

namespace agw::util {

// UUID v4 (random) в формате 8-4-4-4-12, lowercase hex, без фигурных скобок —
// как QUuid::createUuid().toString(QUuid::WithoutBraces). Байты берутся из rng.
std::string makeUuidV4(crypto::IRng &rng);

} // namespace agw::util

#endif // AGW_UTIL_UUID_H
