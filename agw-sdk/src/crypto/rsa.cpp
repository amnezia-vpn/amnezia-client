#include "rsa.h"

#include <memory>
#include <stdexcept>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

namespace agw::crypto
{

    namespace
    {

        using BioPtr = std::unique_ptr<BIO, decltype(&BIO_free)>;
        using PkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
        using PkeyCtxPtr = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;

        PkeyPtr loadPublicKey(const std::string &pem)
        {
            BioPtr bio(BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size())), BIO_free);
            if (!bio) {
                throw std::runtime_error("agw::crypto::rsa: BIO_new_mem_buf failed");
            }
            EVP_PKEY *raw = nullptr;
            if (!PEM_read_bio_PUBKEY(bio.get(), &raw, nullptr, nullptr)) {
                throw std::runtime_error("agw::crypto::rsa: PEM_read_bio_PUBKEY failed");
            }
            return PkeyPtr(raw, EVP_PKEY_free);
        }

        PkeyPtr loadPrivateKey(const std::string &pem)
        {
            BioPtr bio(BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size())), BIO_free);
            if (!bio) {
                throw std::runtime_error("agw::crypto::rsa: BIO_new_mem_buf failed");
            }
            EVP_PKEY *raw = nullptr;
            if (!PEM_read_bio_PrivateKey(bio.get(), &raw, nullptr, nullptr)) {
                throw std::runtime_error("agw::crypto::rsa: PEM_read_bio_PrivateKey failed");
            }
            return PkeyPtr(raw, EVP_PKEY_free);
        }

    } // namespace

    std::vector<std::uint8_t> rsaEncryptPublicPkcs1(const std::vector<std::uint8_t> &plaintext,
                                                    const std::string &publicKeyPem)
    {
        PkeyPtr key = loadPublicKey(publicKeyPem);

        PkeyCtxPtr ctx(EVP_PKEY_CTX_new(key.get(), nullptr), EVP_PKEY_CTX_free);
        if (!ctx) {
            throw std::runtime_error("agw::crypto::rsa: EVP_PKEY_CTX_new failed");
        }
        if (EVP_PKEY_encrypt_init(ctx.get()) != 1) {
            throw std::runtime_error("agw::crypto::rsa: EVP_PKEY_encrypt_init failed");
        }
        if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) != 1) {
            throw std::runtime_error("agw::crypto::rsa: set_rsa_padding failed");
        }

        std::size_t outLen = 0;
        if (EVP_PKEY_encrypt(ctx.get(), nullptr, &outLen, plaintext.data(), plaintext.size()) != 1) {
            throw std::runtime_error("agw::crypto::rsa: EVP_PKEY_encrypt (size) failed");
        }
        std::vector<std::uint8_t> out(outLen);
        if (EVP_PKEY_encrypt(ctx.get(), out.data(), &outLen, plaintext.data(), plaintext.size()) != 1) {
            throw std::runtime_error("agw::crypto::rsa: EVP_PKEY_encrypt failed");
        }
        out.resize(outLen);
        return out;
    }

    bool rsaPublicKeyValid(const std::string &publicKeyPem)
    {
        try {
            loadPublicKey(publicKeyPem);
            return true;
        } catch (...) {
            return false;
        }
    }

    std::vector<std::uint8_t> rsaDecryptPrivatePkcs1(const std::vector<std::uint8_t> &ciphertext,
                                                     const std::string &privateKeyPem)
    {
        PkeyPtr key = loadPrivateKey(privateKeyPem);

        PkeyCtxPtr ctx(EVP_PKEY_CTX_new(key.get(), nullptr), EVP_PKEY_CTX_free);
        if (!ctx) {
            throw std::runtime_error("agw::crypto::rsa: EVP_PKEY_CTX_new failed");
        }
        if (EVP_PKEY_decrypt_init(ctx.get()) != 1) {
            throw std::runtime_error("agw::crypto::rsa: EVP_PKEY_decrypt_init failed");
        }
        if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) != 1) {
            throw std::runtime_error("agw::crypto::rsa: set_rsa_padding failed");
        }

        std::size_t outLen = 0;
        if (EVP_PKEY_decrypt(ctx.get(), nullptr, &outLen, ciphertext.data(), ciphertext.size()) != 1) {
            throw std::runtime_error("agw::crypto::rsa: EVP_PKEY_decrypt (size) failed");
        }
        std::vector<std::uint8_t> out(outLen);
        if (EVP_PKEY_decrypt(ctx.get(), out.data(), &outLen, ciphertext.data(), ciphertext.size()) != 1) {
            throw std::runtime_error("agw::crypto::rsa: EVP_PKEY_decrypt failed");
        }
        out.resize(outLen);
        return out;
    }

} // namespace agw::crypto
