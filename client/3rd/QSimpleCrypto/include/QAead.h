/**
 * Copyright 2021 BrutalWizard (https://github.com/bru74lw1z4rd). All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License"). You may not use
 * this file except in compliance with the License. You can obtain a copy
 * in the file LICENSE in the source distribution
**/

#ifndef QAEAD_H
#define QAEAD_H

#include "QSimpleCrypto_global.h"

#include <QObject>

#include <memory>

#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "QCryptoError.h"

// clang-format off
namespace QSimpleCrypto
{
    class QSIMPLECRYPTO_EXPORT QAead {
    public:
        QAead();

        ///
        /// \brief encryptAesGcm - Function encrypts data with Gcm algorithm.
        /// \param data - Data that will be encrypted.
        /// \param key - AES key.
        /// \param iv - Initialization vector.
        /// \param tag - Authorization tag.
        /// \param aad - Additional authenticated data. Must be nullptr, if not used.
        /// \param cipher - Can be used with OpenSSL EVP_CIPHER (gcm) - 128, 192, 256. Example: EVP_aes_256_gcm().
        /// \return Returns encrypted data or "", if error happened.
        ///
        QByteArray encryptAesGcm(QByteArray data, QByteArray key, QByteArray iv, QByteArray* tag, QByteArray aad = "", const EVP_CIPHER* cipher = EVP_aes_256_gcm());

        ///
        /// \brief decryptAesGcm - Function decrypts data with Gcm algorithm.
        /// \param data - Data that will be decrypted
        /// \param key - AES key
        /// \param iv - Initialization vector
        /// \param tag - Authorization tag
        /// \param aad - Additional authenticated data. Must be nullptr, if not used
        /// \param cipher - Can be used with OpenSSL EVP_CIPHER (gcm) - 128, 192, 256. Example: EVP_aes_256_gcm()
        /// \return Returns decrypted data or "", if error happened.
        ///
        QByteArray decryptAesGcm(QByteArray data, QByteArray key, QByteArray iv, QByteArray* tag, QByteArray aad = "", const EVP_CIPHER* cipher = EVP_aes_256_gcm());

        ///
        /// \brief encryptAesCcm - Function encrypts data with Ccm algorithm.
        /// \param data - Data that will be encrypted.
        /// \param key - AES key.
        /// \param iv - Initialization vector.
        /// \param tag - Authorization tag.
        /// \param aad - Additional authenticated data. Must be nullptr, if not used.
        /// \param cipher - Can be used with OpenSSL EVP_CIPHER (ccm) - 128, 192, 256. Example: EVP_aes_256_ccm().
        /// \return Returns encrypted data or "", if error happened.
        ///
        QByteArray encryptAesCcm(QByteArray data, QByteArray key, QByteArray iv, QByteArray* tag, QByteArray aad = "", const EVP_CIPHER* cipher = EVP_aes_256_ccm());

        ///
        /// \brief decryptAesCcm - Function decrypts data with Ccm algorithm.
        /// \param data - Data that will be decrypted.
        /// \param key - AES key.
        /// \param iv - Initialization vector.
        /// \param tag - Authorization tag.
        /// \param aad - Additional authenticated data. Must be nullptr, if not used.
        /// \param cipher - Can be used with OpenSSL EVP_CIPHER (ccm) - 128, 192, 256. Example: EVP_aes_256_ccm().
        /// \return Returns decrypted data or "", if error happened.
        ///
        QByteArray decryptAesCcm(QByteArray data, QByteArray key, QByteArray iv, QByteArray* tag, QByteArray aad = "", const EVP_CIPHER* cipher = EVP_aes_256_ccm());

        ///
        /// \brief error - Error handler class.
        ///
        QCryptoError error;
    };
} // namespace QSimpleCrypto

#endif // QAEAD_H
