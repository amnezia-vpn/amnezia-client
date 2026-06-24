#include "rng.h"

#include <stdexcept>

#include <openssl/rand.h>

namespace agw::crypto {

std::vector<std::uint8_t> DefaultRng::bytes(std::size_t n)
{
    std::vector<std::uint8_t> out(n);
    if (n == 0) {
        return out;
    }
    if (RAND_priv_bytes(out.data(), static_cast<int>(n)) != 1) {
        throw std::runtime_error("agw::crypto::DefaultRng: RAND_priv_bytes failed");
    }
    return out;
}

} // namespace agw::crypto
