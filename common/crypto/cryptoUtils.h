#ifndef CRYPTOUTILS_H
#define CRYPTOUTILS_H

#include <QByteArray>

#include <openssl/evp.h>

// Thin wrapper around OpenSSL EVP used for local settings encryption and for
// the API-gateway request/response envelope. Replaces the unmaintained
// QSimpleCrypto library. All functions report failures by returning an empty
// QByteArray (or nullptr) and logging, without throwing.
//
// Wire compatibility note: encryptAes256Cbc/decryptAes256Cbc reproduce the exact
// behaviour of QSimpleCrypto::QBlockCipher (AES-256-CBC, PKCS7 padding, first
// 16 bytes of the IV buffer), so data written by previous app versions stays
// readable and gateway requests keep the same format.
namespace CryptoUtils
{
    // Cryptographically secure random bytes of the requested size.
    QByteArray generateRandomBytes(int size);

    // AES-256-CBC with PKCS7 padding. key must be 32 bytes, iv must be at least
    // 16 bytes (only the first 16 are used, matching the legacy behaviour).
    QByteArray encryptAes256Cbc(const QByteArray &data, const QByteArray &key, const QByteArray &iv);
    QByteArray decryptAes256Cbc(const QByteArray &data, const QByteArray &key, const QByteArray &iv);

    // Loads an RSA public key from PEM bytes. Returns nullptr on failure.
    // The caller owns the result and must free it with EVP_PKEY_free().
    EVP_PKEY *loadPublicKeyFromPem(const QByteArray &pem);

    // RSA encryption with the given OpenSSL padding (e.g. RSA_PKCS1_PADDING).
    QByteArray rsaEncrypt(const QByteArray &data, EVP_PKEY *publicKey, int padding);
}

#endif // CRYPTOUTILS_H
