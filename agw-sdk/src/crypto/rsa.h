#ifndef AGW_CRYPTO_RSA_H
#define AGW_CRYPTO_RSA_H

#include <cstdint>
#include <string>
#include <vector>

namespace agw::crypto
{
    std::vector<std::uint8_t> rsaEncryptPublicPkcs1(const std::vector<std::uint8_t> &plaintext,
                                                    const std::string &publicKeyPem);

    std::vector<std::uint8_t> rsaDecryptPrivatePkcs1(const std::vector<std::uint8_t> &ciphertext,
                                                     const std::string &privateKeyPem);

    bool rsaPublicKeyValid(const std::string &publicKeyPem);
}

#endif
