#ifndef AGW_PROTOCOL_RESPONSE_H
#define AGW_PROTOCOL_RESPONSE_H

#include <cstdint>
#include <string>
#include <vector>

namespace agw::protocol
{
    struct DecryptResult
    {
        std::string decryptedBody;
        bool ok = false;
    };

    DecryptResult tryDecryptResponse(const std::string &encrypted, const std::vector<std::uint8_t> &key,
                                     const std::vector<std::uint8_t> &iv);
}

#endif
