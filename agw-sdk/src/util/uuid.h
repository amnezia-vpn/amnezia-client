#ifndef AGW_UTIL_UUID_H
#define AGW_UTIL_UUID_H

#include <string>

#include "crypto/rng.h"

namespace agw::util
{
    std::string makeUuidV4(crypto::IRng &rng);
}

#endif
