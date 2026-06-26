#include "protocol/response.h"

#include "crypto/aes.h"

namespace agw::protocol
{

    DecryptResult tryDecryptResponse(const std::string &encrypted, const std::vector<std::uint8_t> &key,
                                     const std::vector<std::uint8_t> &iv)
    {
        DecryptResult result;
        result.decryptedBody = encrypted;
        result.ok = false;
        try {
            const std::vector<std::uint8_t> in(encrypted.begin(), encrypted.end());
            const std::vector<std::uint8_t> out = crypto::aesDecryptCbc(in, key, iv);
            result.decryptedBody.assign(out.begin(), out.end());
            result.ok = true;
        } catch (...) {
            result.decryptedBody = encrypted;
            result.ok = false;
        }
        return result;
    }

} // namespace agw::protocol
