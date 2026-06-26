#ifndef AGW_PROTOCOL_REQUEST_BUILDER_H
#define AGW_PROTOCOL_REQUEST_BUILDER_H

#include <cstdint>
#include <string>
#include <vector>

#include "agw/types.h"
#include "crypto/rng.h"

namespace agw::protocol
{
    struct EncryptedRequest
    {
        std::string body;
        std::vector<std::uint8_t> key;
        std::vector<std::uint8_t> iv;
        std::vector<std::uint8_t> salt;
        ErrorCode error = ErrorCode::NoError;
    };

    EncryptedRequest buildEncryptedRequest(const std::string &payload, const std::string &publicKeyPem,
                                           crypto::IRng &rng);
}

#endif
