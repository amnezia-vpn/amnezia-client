/**
 * Copyright 2021 BrutalWizard (https://github.com/bru74lw1z4rd). All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License"). You may not use
 * this file except in compliance with the License. You can obtain a copy
 * in the file LICENSE in the source distribution
**/

#ifndef QX509_H
#define QX509_H

#include "QSimpleCrypto_global.h"

#include <QMap>
#include <QObject>

#include <memory>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "QCryptoError.h"

// clang-format off
namespace QSimpleCrypto
{
    class QSIMPLECRYPTO_EXPORT QX509 {

    #define oneYear 31536000L
    #define x509LastVersion 2

    public:
        QX509();

        ///
        /// \brief loadCertificateFromFile - Function load X509 from file and returns OpenSSL structure.
        /// \param fileName - File path to certificate.
        /// \return Returns OpenSSL X509 structure or nullptr, if error happened. Returned value must be cleaned up with 'X509_free' to avoid memory leak.
        ///
        X509* loadCertificateFromFile(const QByteArray& fileName);

        ///
        /// \brief signCertificate - Function signs X509 certificate and returns signed X509 OpenSSL structure.
        /// \param endCertificate - Certificate that will be signed
        /// \param caCertificate - CA certificate that will sign end certificate
        /// \param caPrivateKey - CA certificate private key
        /// \param fileName - With that name certificate will be saved. Leave "", if don't need to save it
        /// \return Returns OpenSSL X509 structure or nullptr, if error happened.
        ///
        X509* signCertificate(X509* endCertificate, X509* caCertificate, EVP_PKEY* caPrivateKey, const QByteArray& fileName = "");

        ///
        /// \brief verifyCertificate - Function verifies X509 certificate and returns verified X509 OpenSSL structure.
        /// \param x509 - OpenSSL X509. That certificate will be verified.
        /// \param store - Trusted certificate must be added to X509_Store with 'addCertificateToStore(X509_STORE* ctx, X509* x509)'.
        /// \return Returns OpenSSL X509 structure or nullptr, if error happened
        ///
        X509* verifyCertificate(X509* x509, X509_STORE* store);

        ///
        /// \brief generateSelfSignedCertificate - Function generatesand returns  self signed X509.
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
        X509* generateSelfSignedCertificate(const RSA* rsa, const QMap<QByteArray, QByteArray>& additionalData,
            const QByteArray& certificateFileName = "", const EVP_MD* md = EVP_sha512(),
            const long& serialNumber = 1, const long& version = x509LastVersion,
            const long& notBefore = 0, const long& notAfter = oneYear);

        ///
        /// \brief error - Error handler class.
        ///
        QCryptoError error;
    };
} // namespace QSimpleCrypto

#endif // QX509_H
