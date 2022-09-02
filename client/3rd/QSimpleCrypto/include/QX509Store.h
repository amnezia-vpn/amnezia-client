/**
 * Copyright 2021 BrutalWizard (https://github.com/bru74lw1z4rd). All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License"). You may not use
 * this file except in compliance with the License. You can obtain a copy
 * in the file LICENSE in the source distribution
**/

#ifndef QX509STORE_H
#define QX509STORE_H

#include "QSimpleCrypto_global.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <memory>

#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>

#include "QCryptoError.h"

// clang-format off
namespace QSimpleCrypto
{
    class QSIMPLECRYPTO_EXPORT QX509Store {
    public:
        QX509Store();

        ///
        /// \brief addCertificateToStore
        /// \param store - OpenSSL X509_STORE.
        /// \param x509 - OpenSSL X509.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool addCertificateToStore(X509_STORE* store, X509* x509);

        ///
        /// \brief addLookup
        /// \param store - OpenSSL X509_STORE.
        /// \param method - OpenSSL X509_LOOKUP_METHOD. Example: X509_LOOKUP_file.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool addLookup(X509_STORE* store, X509_LOOKUP_METHOD* method);

        ///
        /// \brief setCertificateDepth
        /// \param store - OpenSSL X509_STORE.
        /// \param depth - That is the maximum number of untrusted CA certificates that can appear in a chain. Example: 0.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool setDepth(X509_STORE* store, const int& depth);

        ///
        /// \brief setFlag
        /// \param store - OpenSSL X509_STORE.
        /// \param flag - The verification flags consists of zero or more of the following flags ored together. Example: X509_V_FLAG_CRL_CHECK.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool setFlag(X509_STORE* store, const unsigned long& flag);

        ///
        /// \brief setFlag
        /// \param store - OpenSSL X509_STORE.
        /// \param purpose - Verification purpose in param to purpose. Example: X509_PURPOSE_ANY.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool setPurpose(X509_STORE* store, const int& purpose);

        ///
        /// \brief setTrust
        /// \param store - OpenSSL X509_STORE.
        /// \param trust - Trust Level. Example: X509_TRUST_SSL_SERVER.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool setTrust(X509_STORE* store, const int& trust);

        ///
        /// \brief setDefaultPaths
        /// \param store - OpenSSL X509_STORE.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool setDefaultPaths(X509_STORE* store);

        ///
        /// \brief loadLocations
        /// \param store - OpenSSL X509_STORE.
        /// \param fileName - File name. Example: "caCertificate.pem".
        /// \param dirPath - Path to file. Example: "path/To/File".
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool loadLocations(X509_STORE* store, const QByteArray& fileName, const QByteArray& dirPath);

        ///
        /// \brief loadLocations
        /// \param store - OpenSSL X509_STORE.
        /// \param file - Qt QFile that will be loaded.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool loadLocations(X509_STORE* store, const QFile& file);

        ///
        /// \brief loadLocations
        /// \param store - OpenSSL X509_STORE.
        /// \param fileInfo - Qt QFileInfo.
        /// \return Returns 'true' on success and 'false', if error happened.
        ///
        bool loadLocations(X509_STORE* store, const QFileInfo& fileInfo);

        ///
        /// \brief error - Error handler class.
        ///
        QCryptoError error;
    };
}

#endif // QX509STORE_H
