#include "aes.h"

#include <memory>
#include <stdexcept>

#include <openssl/evp.h>

namespace agw::crypto {

namespace {

using CtxPtr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

constexpr int kAes256KeyLen = 32;
constexpr int kAesBlock = 16;

void checkKeyIv(const std::vector<std::uint8_t> &key, const std::vector<std::uint8_t> &iv)
{
    if (key.size() != static_cast<std::size_t>(kAes256KeyLen)) {
        throw std::runtime_error("agw::crypto::aes: key must be 32 bytes (AES-256)");
    }
    if (iv.size() < static_cast<std::size_t>(kAesBlock)) {
        throw std::runtime_error("agw::crypto::aes: iv must be at least 16 bytes");
    }
}

} // namespace

std::vector<std::uint8_t> aesEncryptCbc(const std::vector<std::uint8_t> &data,
                                        const std::vector<std::uint8_t> &key,
                                        const std::vector<std::uint8_t> &iv)
{
    checkKeyIv(key, iv);

    CtxPtr ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) {
        throw std::runtime_error("agw::crypto::aes: EVP_CIPHER_CTX_new failed");
    }

    // iv.data() — CBC прочитает только первые 16 байт, остаток (если есть) игнорируется.
    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        throw std::runtime_error("agw::crypto::aes: EVP_EncryptInit_ex failed");
    }

    std::vector<std::uint8_t> out(data.size() + kAesBlock);
    int len = 0;
    if (EVP_EncryptUpdate(ctx.get(), out.data(), &len,
                          data.data(), static_cast<int>(data.size())) != 1) {
        throw std::runtime_error("agw::crypto::aes: EVP_EncryptUpdate failed");
    }
    int total = len;

    if (EVP_EncryptFinal_ex(ctx.get(), out.data() + total, &len) != 1) {
        throw std::runtime_error("agw::crypto::aes: EVP_EncryptFinal_ex failed");
    }
    total += len;

    out.resize(static_cast<std::size_t>(total));
    return out;
}

std::vector<std::uint8_t> aesDecryptCbc(const std::vector<std::uint8_t> &data,
                                        const std::vector<std::uint8_t> &key,
                                        const std::vector<std::uint8_t> &iv)
{
    checkKeyIv(key, iv);

    CtxPtr ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) {
        throw std::runtime_error("agw::crypto::aes: EVP_CIPHER_CTX_new failed");
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        throw std::runtime_error("agw::crypto::aes: EVP_DecryptInit_ex failed");
    }

    std::vector<std::uint8_t> out(data.size() + kAesBlock);
    int len = 0;
    if (EVP_DecryptUpdate(ctx.get(), out.data(), &len,
                          data.data(), static_cast<int>(data.size())) != 1) {
        throw std::runtime_error("agw::crypto::aes: EVP_DecryptUpdate failed");
    }
    int total = len;

    if (EVP_DecryptFinal_ex(ctx.get(), out.data() + total, &len) != 1) {
        throw std::runtime_error("agw::crypto::aes: EVP_DecryptFinal_ex failed (bad key/iv/padding)");
    }
    total += len;

    out.resize(static_cast<std::size_t>(total));
    return out;
}

} // namespace agw::crypto
