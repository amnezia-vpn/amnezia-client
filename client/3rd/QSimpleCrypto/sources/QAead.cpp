/**
 * Copyright 2021 BrutalWizard (https://github.com/bru74lw1z4rd). All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License"). You may not use
 * this file except in compliance with the License. You can obtain a copy
 * in the file LICENSE in the source distribution
**/

#include "include/QAead.h"

QSimpleCrypto::QAead::QAead()
{
}

///
/// \brief QSimpleCrypto::QAEAD::encryptAesGcm - Function encrypts data with Gcm algorithm.
/// \param data - Data that will be encrypted.
/// \param key - AES key.
/// \param iv - Initialization vector.
/// \param tag - Authorization tag.
/// \param aad - Additional authenticated data. Must be nullptr, if not used.
/// \param cipher - Can be used with OpenSSL EVP_CIPHER (gcm) - 128, 192, 256. Example: EVP_aes_256_gcm().
/// \return Returns encrypted data or "", if error happened.
///
QByteArray QSimpleCrypto::QAead::encryptAesGcm(QByteArray data, QByteArray key, QByteArray iv, QByteArray* tag, QByteArray aad, const EVP_CIPHER* cipher)
{
    try {
        /* Initialize EVP_CIPHER_CTX */
        std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX*)> encryptionCipher { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };
        if (encryptionCipher == nullptr) {
            throw std::runtime_error("Couldn't initialize \'encryptionCipher\'. EVP_CIPHER_CTX_new(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set data length */
        int plainTextLength = data.size();
        int cipherTextLength = 0;

        /* Initialize cipherText. Here encrypted data will be stored */
        std::unique_ptr<unsigned char[]> cipherText { new unsigned char[plainTextLength]() };
        if (cipherText == nullptr) {
            throw std::runtime_error("Couldn't allocate memory for 'ciphertext'.");
        }

        /* Initialize encryption operation. */
        if (!EVP_EncryptInit_ex(encryptionCipher.get(), cipher, nullptr, reinterpret_cast<unsigned char*>(key.data()), reinterpret_cast<unsigned char*>(iv.data()))) {
            throw std::runtime_error("Couldn't initialize encryption operation. EVP_EncryptInit_ex(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set IV length if default 12 bytes (96 bits) is not appropriate */
        if (!EVP_CIPHER_CTX_ctrl(encryptionCipher.get(), EVP_CTRL_GCM_SET_IVLEN, iv.length(), nullptr)) {
            throw std::runtime_error("Couldn't set IV length. EVP_CIPHER_CTX_ctrl(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

//        /* Check if aad need to be used */
//        if (aad.length() > 0) {
//            /* Provide any AAD data. This can be called zero or more times as required */
//            if (!EVP_EncryptUpdate(encryptionCipher.get(), nullptr, &cipherTextLength, reinterpret_cast<unsigned char*>(aad.data()), aad.length())) {
//                throw std::runtime_error("Couldn't provide aad data. EVP_EncryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
//            }
//        }

        /*
         * Provide the message to be encrypted, and obtain the encrypted output.
         * EVP_EncryptUpdate can be called multiple times if necessary
        */
        if (!EVP_EncryptUpdate(encryptionCipher.get(), cipherText.get(), &cipherTextLength, reinterpret_cast<const unsigned char*>(data.data()), plainTextLength)) {
            throw std::runtime_error("Couldn't provide message to be encrypted. EVP_EncryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /*
         * Finalize the encryption. Normally cipher text bytes may be written at
         * this stage, but this does not occur in GCM mode
        */
        if (!EVP_EncryptFinal_ex(encryptionCipher.get(), cipherText.get(), &plainTextLength)) {
            throw std::runtime_error("Couldn't finalize encryption. EVP_EncryptFinal_ex(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

//        /* Get tag */
//        if (!EVP_CIPHER_CTX_ctrl(encryptionCipher.get(), EVP_CTRL_GCM_GET_TAG, tag->length(), reinterpret_cast<unsigned char*>(tag->data()))) {
//            throw std::runtime_error("Couldn't get tag. EVP_CIPHER_CTX_ctrl(. Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
//        }

        /* Finilize data to be readable with qt */
        QByteArray encryptedData = QByteArray(reinterpret_cast<char*>(cipherText.get()), cipherTextLength);

        return encryptedData;

    } catch (std::exception& exception) {
        QSimpleCrypto::QAead::error.setError(1, exception.what());
        return QByteArray();
    } catch (...) {
        QSimpleCrypto::QAead::error.setError(2, "Unknown error!");
        return QByteArray();
    }

    return QByteArray();
}

///
/// \brief QSimpleCrypto::QAEAD::decryptAesGcm - Function decrypts data with Gcm algorithm.
/// \param data - Data that will be decrypted
/// \param key - AES key
/// \param iv - Initialization vector
/// \param tag - Authorization tag
/// \param aad - Additional authenticated data. Must be nullptr, if not used
/// \param cipher - Can be used with OpenSSL EVP_CIPHER (gcm) - 128, 192, 256. Example: EVP_aes_256_gcm()
/// \return Returns decrypted data or "", if error happened.
///
QByteArray QSimpleCrypto::QAead::decryptAesGcm(QByteArray data, QByteArray key, QByteArray iv, QByteArray* tag, QByteArray aad, const EVP_CIPHER* cipher)
{
    try {
        /* Initialize EVP_CIPHER_CTX */
        std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX*)> decryptionCipher { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };
        if (decryptionCipher.get() == nullptr) {
            throw std::runtime_error("Couldn't initialize \'decryptionCipher\'. EVP_CIPHER_CTX_new(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set data length */
        int cipherTextLength = data.size();
        int plainTextLength = 0;

        /* Initialize plainText. Here decrypted data will be stored */
        std::unique_ptr<unsigned char[]> plainText { new unsigned char[cipherTextLength]() };
        if (plainText == nullptr) {
            throw std::runtime_error("Couldn't allocate memory for 'plaintext'.");
        }

        /* Initialize decryption operation. */
        if (!EVP_DecryptInit_ex(decryptionCipher.get(), cipher, nullptr, reinterpret_cast<unsigned char*>(key.data()), reinterpret_cast<unsigned char*>(iv.data()))) {
            throw std::runtime_error("Couldn't initialize decryption operation. EVP_DecryptInit_ex(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
        if (!EVP_CIPHER_CTX_ctrl(decryptionCipher.get(), EVP_CTRL_GCM_SET_IVLEN, iv.length(), nullptr)) {
            throw std::runtime_error("Couldn't set IV length. EVP_CIPHER_CTX_ctrl(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

//        /* Check if aad need to be used */
//        if (aad.length() > 0) {
//            /* Provide any AAD data. This can be called zero or more times as required */
//            if (!EVP_DecryptUpdate(decryptionCipher.get(), nullptr, &plainTextLength, reinterpret_cast<unsigned char*>(aad.data()), aad.length())) {
//                throw std::runtime_error("Couldn't provide aad data. EVP_DecryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
//            }
//        }

        /*
         * Provide the message to be decrypted, and obtain the plain text output.
         * EVP_DecryptUpdate can be called multiple times if necessary
        */
        if (!EVP_DecryptUpdate(decryptionCipher.get(), plainText.get(), &plainTextLength, reinterpret_cast<const unsigned char*>(data.data()), cipherTextLength)) {
            throw std::runtime_error("Couldn't provide message to be decrypted. EVP_DecryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

//        /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
//        if (!EVP_CIPHER_CTX_ctrl(decryptionCipher.get(), EVP_CTRL_GCM_SET_TAG, tag->length(), reinterpret_cast<unsigned char*>(tag->data()))) {
//            throw std::runtime_error("Coldn't set tag. EVP_CIPHER_CTX_ctrl(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
//        }

        /*
         * Finalize the decryption. A positive return value indicates success,
         * anything else is a failure - the plain text is not trustworthy.
        */
        if (!EVP_DecryptFinal_ex(decryptionCipher.get(), plainText.get(), &cipherTextLength)) {
            throw std::runtime_error("Couldn't finalize decryption. EVP_DecryptFinal_ex(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Finilize data to be readable with qt */
        QByteArray decryptedData = QByteArray(reinterpret_cast<char*>(plainText.get()), plainTextLength);

        return decryptedData;

    } catch (std::exception& exception) {
        QSimpleCrypto::QAead::error.setError(1, exception.what());
        return QByteArray();
    } catch (...) {
        QSimpleCrypto::QAead::error.setError(2, "Unknown error!");
        return QByteArray();
    }

    return QByteArray();
}

///
/// \brief QSimpleCrypto::QAEAD::encryptAesCcm - Function encrypts data with Ccm algorithm.
/// \param data - Data that will be encrypted.
/// \param key - AES key.
/// \param iv - Initialization vector.
/// \param tag - Authorization tag.
/// \param aad - Additional authenticated data. Must be nullptr, if not used.
/// \param cipher - Can be used with OpenSSL EVP_CIPHER (ccm) - 128, 192, 256. Example: EVP_aes_256_ccm().
/// \return Returns encrypted data or "", if error happened.
///
QByteArray QSimpleCrypto::QAead::encryptAesCcm(QByteArray data, QByteArray key, QByteArray iv, QByteArray* tag, QByteArray aad, const EVP_CIPHER* cipher)
{
    try {
        /* Initialize EVP_CIPHER_CTX */
        std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX*)> encryptionCipher { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };
        if (encryptionCipher == nullptr) {
            throw std::runtime_error("Couldn't initialize \'encryptionCipher\'. EVP_CIPHER_CTX_new(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set data length */
        int plainTextLength = data.size();
        int cipherTextLength = 0;

        /* Initialize cipherText. Here encrypted data will be stored */
        std::unique_ptr<unsigned char[]> cipherText { new unsigned char[plainTextLength]() };
        if (cipherText.get() == nullptr) {
            throw std::runtime_error("Couldn't allocate memory for 'ciphertext'.");
        }

        /* Initialize encryption operation. */
        if (!EVP_EncryptInit_ex(encryptionCipher.get(), cipher, nullptr, reinterpret_cast<unsigned char*>(key.data()), reinterpret_cast<unsigned char*>(iv.data()))) {
            throw std::runtime_error("Couldn't initialize encryption operation. EVP_EncryptInit_ex(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set IV length if default 12 bytes (96 bits) is not appropriate */
        if (!EVP_CIPHER_CTX_ctrl(encryptionCipher.get(), EVP_CTRL_CCM_SET_IVLEN, iv.length(), nullptr)) {
            throw std::runtime_error("Couldn't set IV length. EVP_CIPHER_CTX_ctrl(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set tag length */
        if (!EVP_CIPHER_CTX_ctrl(encryptionCipher.get(), EVP_CTRL_CCM_SET_TAG, tag->length(), nullptr)) {
            throw std::runtime_error("Coldn't set tag. EVP_CIPHER_CTX_ctrl(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Check if aad need to be used */
        if (aad.length() > 0) {
            /* Provide the total plain text length */
            if (!EVP_EncryptUpdate(encryptionCipher.get(), nullptr, &cipherTextLength, nullptr, plainTextLength)) {
                throw std::runtime_error("Couldn't provide total plaintext length. EVP_EncryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }

            /* Provide any AAD data. This can be called zero or more times as required */
            if (!EVP_EncryptUpdate(encryptionCipher.get(), nullptr, &cipherTextLength, reinterpret_cast<unsigned char*>(aad.data()), aad.length())) {
                throw std::runtime_error("Couldn't provide aad data. EVP_EncryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }
        }

        /*
         * Provide the message to be encrypted, and obtain the encrypted output.
         * EVP_EncryptUpdate can be called multiple times if necessary
        */
        if (!EVP_EncryptUpdate(encryptionCipher.get(), cipherText.get(), &cipherTextLength, reinterpret_cast<const unsigned char*>(data.data()), plainTextLength)) {
            throw std::runtime_error("Couldn't provide message to be encrypted. EVP_EncryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /*
         * Finalize the encryption. Normally ciphertext bytes may be written at
         * this stage, but this does not occur in GCM mode
        */
        if (!EVP_EncryptFinal_ex(encryptionCipher.get(), cipherText.get(), &plainTextLength)) {
            throw std::runtime_error("Couldn't finalize encryption. EVP_EncryptFinal_ex(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Get tag */
        if (!EVP_CIPHER_CTX_ctrl(encryptionCipher.get(), EVP_CTRL_CCM_GET_TAG, tag->length(), reinterpret_cast<unsigned char*>(tag->data()))) {
            throw std::runtime_error("Couldn't get tag. EVP_CIPHER_CTX_ctrl(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Finilize data to be readable with qt */
        QByteArray encryptedData = QByteArray(reinterpret_cast<char*>(cipherText.get()), cipherTextLength);

        return encryptedData;

    } catch (std::exception& exception) {
        QSimpleCrypto::QAead::error.setError(1, exception.what());
        return QByteArray();
    } catch (...) {
        QSimpleCrypto::QAead::error.setError(2, "Unknown error!");
        return QByteArray();
    }

    return QByteArray();
}

///
/// \brief QSimpleCrypto::QAEAD::decryptAesCcm - Function decrypts data with Ccm algorithm.
/// \param data - Data that will be decrypted.
/// \param key - AES key.
/// \param iv - Initialization vector.
/// \param tag - Authorization tag.
/// \param aad - Additional authenticated data. Must be nullptr, if not used.
/// \param cipher - Can be used with OpenSSL EVP_CIPHER (ccm) - 128, 192, 256. Example: EVP_aes_256_ccm().
/// \return Returns decrypted data or "", if error happened.
///
QByteArray QSimpleCrypto::QAead::decryptAesCcm(QByteArray data, QByteArray key, QByteArray iv, QByteArray* tag, QByteArray aad, const EVP_CIPHER* cipher)
{
    try {
        /* Initialize EVP_CIPHER_CTX */
        std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX*)> decryptionCipher { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };
        if (decryptionCipher.get() == nullptr) {
            throw std::runtime_error("Couldn't initialize \'decryptionCipher\'. EVP_CIPHER_CTX_new(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set data length */
        int cipherTextLength = data.size();
        int plainTextLength = 0;

        /* Initialize plainText. Here decrypted data will be stored */
        std::unique_ptr<unsigned char[]> plainText { new unsigned char[cipherTextLength]() };
        if (plainText == nullptr) {
            throw std::runtime_error("Couldn't allocate memory for 'plaintext'.");
        }

        /* Initialize decryption operation. */
        if (!EVP_DecryptInit_ex(decryptionCipher.get(), cipher, nullptr, reinterpret_cast<unsigned char*>(key.data()), reinterpret_cast<unsigned char*>(iv.data()))) {
            throw std::runtime_error("Couldn't initialize decryption operation. EVP_DecryptInit_ex(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
        if (!EVP_CIPHER_CTX_ctrl(decryptionCipher.get(), EVP_CTRL_CCM_SET_IVLEN, iv.length(), nullptr)) {
            throw std::runtime_error("Couldn't set IV length. EVP_CIPHER_CTX_ctrl(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
        if (!EVP_CIPHER_CTX_ctrl(decryptionCipher.get(), EVP_CTRL_CCM_SET_TAG, tag->length(), reinterpret_cast<unsigned char*>(tag->data()))) {
            throw std::runtime_error("Coldn't set tag. EVP_CIPHER_CTX_ctrl(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Check if aad need to be used */
        if (aad.length() > 0) {
            /* Provide the total ciphertext length */
            if (!EVP_DecryptUpdate(decryptionCipher.get(), nullptr, &plainTextLength, nullptr, cipherTextLength)) {
                throw std::runtime_error("Couldn't provide total plaintext length. EVP_DecryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }

            /* Provide any AAD data. This can be called zero or more times as required */
            if (!EVP_DecryptUpdate(decryptionCipher.get(), nullptr, &plainTextLength, reinterpret_cast<unsigned char*>(aad.data()), aad.length())) {
                throw std::runtime_error("Couldn't provide aad data. EVP_DecryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }
        }

        /*
         * Provide the message to be decrypted, and obtain the plaintext output.
         * EVP_DecryptUpdate can be called multiple times if necessary
        */
        if (!EVP_DecryptUpdate(decryptionCipher.get(), plainText.get(), &plainTextLength, reinterpret_cast<const unsigned char*>(data.data()), cipherTextLength)) {
            throw std::runtime_error("Couldn't provide message to be decrypted. EVP_DecryptUpdate(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /*
         * Finalize the decryption. A positive return value indicates success,
         * anything else is a failure - the plaintext is not trustworthy.
        */
        if (!EVP_DecryptFinal_ex(decryptionCipher.get(), plainText.get(), &cipherTextLength)) {
            throw std::runtime_error("Couldn't finalize decryption. EVP_DecryptFinal_ex(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Finilize data to be readable with qt */
        QByteArray decryptedData = QByteArray(reinterpret_cast<char*>(plainText.get()), plainTextLength);

        return decryptedData;

    } catch (std::exception& exception) {
        QSimpleCrypto::QAead::error.setError(1, exception.what());
        return QByteArray();
    } catch (...) {
        QSimpleCrypto::QAead::error.setError(2, "Unknown error!");
        return QByteArray();
    }

    return QByteArray();
}
