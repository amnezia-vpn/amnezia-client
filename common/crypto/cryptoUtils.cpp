#include "cryptoUtils.h"

#include <memory>

#include <QDebug>

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

namespace
{
    constexpr int kAesKeySize = 32;   // AES-256
    constexpr int kAesIvSize = 16;    // CBC block size
    constexpr int kAesBlockSize = 16;

    QByteArray lastOpenSslError()
    {
        return QByteArray(ERR_error_string(ERR_get_error(), nullptr));
    }
}

QByteArray CryptoUtils::generateRandomBytes(int size)
{
    if (size <= 0) {
        qCritical() << "CryptoUtils::generateRandomBytes invalid size" << size;
        return {};
    }

    QByteArray buffer(size, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char *>(buffer.data()), size) != 1) {
        qCritical() << "CryptoUtils::generateRandomBytes RAND_bytes failed:" << lastOpenSslError();
        return {};
    }

    return buffer;
}

QByteArray CryptoUtils::encryptAes256Cbc(const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    if (key.size() != kAesKeySize || iv.size() < kAesIvSize) {
        qCritical() << "CryptoUtils::encryptAes256Cbc invalid key/iv size" << key.size() << iv.size();
        return {};
    }

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };
    if (!ctx) {
        qCritical() << "CryptoUtils::encryptAes256Cbc EVP_CIPHER_CTX_new failed:" << lastOpenSslError();
        return {};
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(key.constData()),
                           reinterpret_cast<const unsigned char *>(iv.constData()))
        != 1) {
        qCritical() << "CryptoUtils::encryptAes256Cbc EVP_EncryptInit_ex failed:" << lastOpenSslError();
        return {};
    }

    QByteArray cipherText(data.size() + kAesBlockSize, Qt::Uninitialized);
    int length = 0;
    int cipherTextLength = 0;

    if (EVP_EncryptUpdate(ctx.get(), reinterpret_cast<unsigned char *>(cipherText.data()), &length,
                          reinterpret_cast<const unsigned char *>(data.constData()), data.size())
        != 1) {
        qCritical() << "CryptoUtils::encryptAes256Cbc EVP_EncryptUpdate failed:" << lastOpenSslError();
        return {};
    }
    cipherTextLength = length;

    if (EVP_EncryptFinal_ex(ctx.get(), reinterpret_cast<unsigned char *>(cipherText.data()) + cipherTextLength, &length) != 1) {
        qCritical() << "CryptoUtils::encryptAes256Cbc EVP_EncryptFinal_ex failed:" << lastOpenSslError();
        return {};
    }
    cipherTextLength += length;

    cipherText.resize(cipherTextLength);
    return cipherText;
}

QByteArray CryptoUtils::decryptAes256Cbc(const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    if (key.size() != kAesKeySize || iv.size() < kAesIvSize) {
        qCritical() << "CryptoUtils::decryptAes256Cbc invalid key/iv size" << key.size() << iv.size();
        return {};
    }

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };
    if (!ctx) {
        qCritical() << "CryptoUtils::decryptAes256Cbc EVP_CIPHER_CTX_new failed:" << lastOpenSslError();
        return {};
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(key.constData()),
                           reinterpret_cast<const unsigned char *>(iv.constData()))
        != 1) {
        qCritical() << "CryptoUtils::decryptAes256Cbc EVP_DecryptInit_ex failed:" << lastOpenSslError();
        return {};
    }

    QByteArray plainText(data.size() + kAesBlockSize, Qt::Uninitialized);
    int length = 0;
    int plainTextLength = 0;

    if (EVP_DecryptUpdate(ctx.get(), reinterpret_cast<unsigned char *>(plainText.data()), &length,
                          reinterpret_cast<const unsigned char *>(data.constData()), data.size())
        != 1) {
        qCritical() << "CryptoUtils::decryptAes256Cbc EVP_DecryptUpdate failed:" << lastOpenSslError();
        return {};
    }
    plainTextLength = length;

    if (EVP_DecryptFinal_ex(ctx.get(), reinterpret_cast<unsigned char *>(plainText.data()) + plainTextLength, &length) != 1) {
        qCritical() << "CryptoUtils::decryptAes256Cbc EVP_DecryptFinal_ex failed:" << lastOpenSslError();
        return {};
    }
    plainTextLength += length;

    plainText.resize(plainTextLength);
    return plainText;
}

EVP_PKEY *CryptoUtils::loadPublicKeyFromPem(const QByteArray &pem)
{
    std::unique_ptr<BIO, decltype(&BIO_free)> bio { BIO_new_mem_buf(pem.constData(), pem.size()), BIO_free };
    if (!bio) {
        qCritical() << "CryptoUtils::loadPublicKeyFromPem BIO_new_mem_buf failed:" << lastOpenSslError();
        return nullptr;
    }

    EVP_PKEY *key = PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);
    if (!key) {
        qCritical() << "CryptoUtils::loadPublicKeyFromPem PEM_read_bio_PUBKEY failed:" << lastOpenSslError();
        return nullptr;
    }

    return key;
}

QByteArray CryptoUtils::rsaEncrypt(const QByteArray &data, EVP_PKEY *publicKey, int padding)
{
    if (!publicKey) {
        qCritical() << "CryptoUtils::rsaEncrypt public key is null";
        return {};
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx { EVP_PKEY_CTX_new(publicKey, nullptr), EVP_PKEY_CTX_free };
    if (!ctx) {
        qCritical() << "CryptoUtils::rsaEncrypt EVP_PKEY_CTX_new failed:" << lastOpenSslError();
        return {};
    }

    if (EVP_PKEY_encrypt_init(ctx.get()) != 1) {
        qCritical() << "CryptoUtils::rsaEncrypt EVP_PKEY_encrypt_init failed:" << lastOpenSslError();
        return {};
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), padding) != 1) {
        qCritical() << "CryptoUtils::rsaEncrypt EVP_PKEY_CTX_set_rsa_padding failed:" << lastOpenSslError();
        return {};
    }

    const auto *plainData = reinterpret_cast<const unsigned char *>(data.constData());

    std::size_t cipherTextLength = 0;
    if (EVP_PKEY_encrypt(ctx.get(), nullptr, &cipherTextLength, plainData, data.size()) != 1) {
        qCritical() << "CryptoUtils::rsaEncrypt couldn't determine buffer length:" << lastOpenSslError();
        return {};
    }

    QByteArray cipherText(static_cast<int>(cipherTextLength), Qt::Uninitialized);
    if (EVP_PKEY_encrypt(ctx.get(), reinterpret_cast<unsigned char *>(cipherText.data()), &cipherTextLength, plainData, data.size())
        != 1) {
        qCritical() << "CryptoUtils::rsaEncrypt EVP_PKEY_encrypt failed:" << lastOpenSslError();
        return {};
    }

    cipherText.resize(static_cast<int>(cipherTextLength));
    return cipherText;
}
