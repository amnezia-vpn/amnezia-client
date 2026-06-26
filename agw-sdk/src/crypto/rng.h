#ifndef AGW_CRYPTO_RNG_H
#define AGW_CRYPTO_RNG_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace agw::crypto
{
    class IRng
    {
    public:
        virtual ~IRng() = default;
        virtual std::vector<std::uint8_t> bytes(std::size_t n) = 0;
    };

    class DefaultRng : public IRng
    {
    public:
        std::vector<std::uint8_t> bytes(std::size_t n) override;
    };
}

#endif
