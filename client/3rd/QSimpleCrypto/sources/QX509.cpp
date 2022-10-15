/**
 * Copyright 2021 BrutalWizard (https://github.com/bru74lw1z4rd). All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License"). You may not use
 * this file except in compliance with the License. You can obtain a copy
 * in the file LICENSE in the source distribution
**/

#include "include/QX509.h"

QSimpleCrypto::QX509::QX509()
{
}

///
/// \brief QSimpleCrypto::QX509::loadCertificateFromFile - Function load X509 from file and returns OpenSSL structure.
/// \param fileName - File path to certificate.
/// \return Returns OpenSSL X509 structure or nullptr, if error happened. Returned value must be cleaned up with 'X509_free' to avoid memory leak.
///
X509* QSimpleCrypto::QX509::loadCertificateFromFile(const QByteArray& fileName)
{
    try {
        /* Initialize X509 */
        X509* x509 = nullptr;
        if (!(x509 = X509_new())) {
            throw std::runtime_error("Couldn't initialize X509. X509_new(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Initialize BIO */
        std::unique_ptr<BIO, void (*)(BIO*)> certFile { BIO_new_file(fileName.data(), "r+"), BIO_free_all };
        if (certFile == nullptr) {
            throw std::runtime_error("Couldn't initialize certFile. BIO_new_file(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Read file */
        if (!PEM_read_bio_X509(certFile.get(), &x509, nullptr, nullptr)) {
            throw std::runtime_error("Couldn't read certificate file from disk. PEM_read_bio_X509(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        return x509;
    } catch (std::exception& exception) {
        QSimpleCrypto::QX509::error.setError(1, exception.what());
        return nullptr;
    } catch (...) {
        QSimpleCrypto::QX509::error.setError(2, "Unknown error!");
        return nullptr;
    }
}

///
/// \brief QSimpleCrypto::QX509::signCertificate - Function signs X509 certificate and returns signed X509 OpenSSL structure.
/// \param endCertificate - Certificate that will be signed
/// \param caCertificate - CA certificate that will sign end certificate
/// \param caPrivateKey - CA certificate private key
/// \param fileName - With that name certificate will be saved. Leave "", if don't need to save it
/// \return Returns OpenSSL X509 structure or nullptr, if error happened.
///
X509* QSimpleCrypto::QX509::signCertificate(X509* endCertificate, X509* caCertificate, EVP_PKEY* caPrivateKey, const QByteArray& fileName)
{
    try {
        /* Set issuer to CA's subject. */
        if (!X509_set_issuer_name(endCertificate, X509_get_subject_name(caCertificate))) {
            throw std::runtime_error("Couldn't set issuer name for X509. X509_set_issuer_name(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Sign the certificate with key. */
        if (!X509_sign(endCertificate, caPrivateKey, EVP_sha256())) {
            throw std::runtime_error("Couldn't sign X509. X509_sign(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Write certificate file on disk. If needed */
        if (!fileName.isEmpty()) {
            /* Initialize BIO */
            std::unique_ptr<BIO, void (*)(BIO*)> certFile { BIO_new_file(fileName.data(), "w+"), BIO_free_all };
            if (certFile == nullptr) {
                throw std::runtime_error("Couldn't initialize certFile. BIO_new_file(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }

            /* Write file on disk */
            if (!PEM_write_bio_X509(certFile.get(), endCertificate)) {
                throw std::runtime_error("Couldn't write certificate file on disk. PEM_write_bio_X509(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }
        }

        return endCertificate;
    } catch (std::exception& exception) {
        QSimpleCrypto::QX509::error.setError(1, exception.what());
        return nullptr;
    } catch (...) {
        QSimpleCrypto::QX509::error.setError(2, "Unknown error!");
        return nullptr;
    }
}

///
/// \brief QSimpleCrypto::QX509::verifyCertificate - Function verifies X509 certificate and returns verified X509 OpenSSL structure.
/// \param x509 - OpenSSL X509. That certificate will be verified.
/// \param store - Trusted certificate must be added to X509_Store with 'addCertificateToStore(X509_STORE* ctx, X509* x509)'.
/// \return Returns OpenSSL X509 structure or nullptr, if error happened
///
X509* QSimpleCrypto::QX509::verifyCertificate(X509* x509, X509_STORE* store)
{
    try {
        /* Initialize X509_STORE_CTX */
        std::unique_ptr<X509_STORE_CTX, void (*)(X509_STORE_CTX*)> ctx { X509_STORE_CTX_new(), X509_STORE_CTX_free };
        if (ctx == nullptr) {
            throw std::runtime_error("Couldn't initialize keyStore. EVP_PKEY_new(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set up CTX for a subsequent verification operation */
        if (!X509_STORE_CTX_init(ctx.get(), store, x509, nullptr)) {
            throw std::runtime_error("Couldn't initialize X509_STORE_CTX. X509_STORE_CTX_init(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Verify X509 */
        if (!X509_verify_cert(ctx.get())) {
            throw std::runtime_error("Couldn't verify cert. X509_verify_cert(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        return x509;
    } catch (std::exception& exception) {
        QSimpleCrypto::QX509::error.setError(1, exception.what());
        return nullptr;
    } catch (...) {
        QSimpleCrypto::QX509::error.setError(2, "Unknown error!");
        return nullptr;
    }
}

///
/// \brief QSimpleCrypto::QX509::generateSelfSignedCertificate - Function generatesand returns  self signed X509.
/// \param rsa - OpenSSL RSA.
/// \param additionalData - Certificate information.
/// \param certificateFileName - With that name certificate will be saved. Leave "", if don't need to save it.
/// \param md - OpenSSL EVP_MD structure. Example: EVP_sha512().
/// \param serialNumber - X509 certificate serial number.
/// \param version - X509 certificate version.
/// \param notBefore - X509 start date.
/// \param notAfter - X509 end date.
/// \return Returns OpenSSL X509 structure or nullptr, if error happened. Returned value must be cleaned up with 'X509_free' to avoid memory leak.
///
X509* QSimpleCrypto::QX509::generateSelfSignedCertificate(const RSA* rsa, const QMap<QByteArray, QByteArray>& additionalData,
    const QByteArray& certificateFileName, const EVP_MD* md,
    const long& serialNumber, const long& version,
    const long& notBefore, const long& notAfter)
{
    try {
        /* Initialize X509 */
        X509* x509 = nullptr;
        if (!(x509 = X509_new())) {
            throw std::runtime_error("Couldn't initialize X509. X509_new(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Initialize EVP_PKEY */
        std::unique_ptr<EVP_PKEY, void (*)(EVP_PKEY*)> keyStore { EVP_PKEY_new(), EVP_PKEY_free };
        if (keyStore == nullptr) {
            throw std::runtime_error("Couldn't initialize keyStore. EVP_PKEY_new(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Sign rsa key */
        if (!EVP_PKEY_assign_RSA(keyStore.get(), rsa)) {
            throw std::runtime_error("Couldn't assign rsa. EVP_PKEY_assign_RSA(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set certificate serial number. */
        if (!ASN1_INTEGER_set(X509_get_serialNumber(x509), serialNumber)) {
            throw std::runtime_error("Couldn't set serial number. ASN1_INTEGER_set(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set certificate version */
        if (!X509_set_version(x509, version)) {
            throw std::runtime_error("Couldn't set version. X509_set_version(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Set certificate creation and expiration date */
        X509_gmtime_adj(X509_get_notBefore(x509), notBefore);
        X509_gmtime_adj(X509_get_notAfter(x509), notAfter);

        /* Set certificate public key */
        if (!X509_set_pubkey(x509, keyStore.get())) {
            throw std::runtime_error("Couldn't set public key. X509_set_pubkey(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Initialize X509_NAME */
        X509_NAME* x509Name = X509_get_subject_name(x509);
        if (x509Name == nullptr) {
            throw std::runtime_error("Couldn't initialize X509_NAME. X509_NAME(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Add additional data to certificate */
        QMapIterator<QByteArray, QByteArray> certificateInformationList(additionalData);
        while (certificateInformationList.hasNext()) {
            /* Read next item in list */
            certificateInformationList.next();

            /* Set additional data */
            if (!X509_NAME_add_entry_by_txt(x509Name, certificateInformationList.key().data(), MBSTRING_UTF8, reinterpret_cast<const unsigned char*>(certificateInformationList.value().data()), -1, -1, 0)) {
                throw std::runtime_error("Couldn't set additional information. X509_NAME_add_entry_by_txt(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }
        }

        /* Set certificate info */
        if (!X509_set_issuer_name(x509, x509Name)) {
            throw std::runtime_error("Couldn't set issuer name. X509_set_issuer_name(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Sign certificate */
        if (!X509_sign(x509, keyStore.get(), md)) {
            throw std::runtime_error("Couldn't sign X509. X509_sign(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        }

        /* Write certificate file on disk. If needed */
        if (!certificateFileName.isEmpty()) {
            /* Initialize BIO */
            std::unique_ptr<BIO, void (*)(BIO*)> certFile { BIO_new_file(certificateFileName.data(), "w+"), BIO_free_all };
            if (certFile == nullptr) {
                throw std::runtime_error("Couldn't initialize certFile. BIO_new_file(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }

            /* Write file on disk */
            if (!PEM_write_bio_X509(certFile.get(), x509)) {
                throw std::runtime_error("Couldn't write certificate file on disk. PEM_write_bio_X509(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
            }
        }

        return x509;
    } catch (std::exception& exception) {
        QSimpleCrypto::QX509::error.setError(1, exception.what());
        return nullptr;
    } catch (...) {
        QSimpleCrypto::QX509::error.setError(2, "Unknown error!");
        return nullptr;
    }
}
