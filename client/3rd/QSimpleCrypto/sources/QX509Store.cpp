/**
 * Copyright 2021 BrutalWizard (https://github.com/bru74lw1z4rd). All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License"). You may not use
 * this file except in compliance with the License. You can obtain a copy
 * in the file LICENSE in the source distribution
**/

#include "include/QX509Store.h"

QSimpleCrypto::QX509Store::QX509Store()
{
}

///
/// \brief QSimpleCrypto::QX509::addCertificateToStore
/// \param store - OpenSSL X509_STORE.
/// \param x509 - OpenSSL X509.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::addCertificateToStore(X509_STORE* store, X509* x509)
{
    if (!X509_STORE_add_cert(store, x509)) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't add certificate to X509_STORE. X509_STORE_add_cert(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::addLookup
/// \param store - OpenSSL X509_STORE.
/// \param method - OpenSSL X509_LOOKUP_METHOD. Example: X509_LOOKUP_file.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::addLookup(X509_STORE* store, X509_LOOKUP_METHOD* method)
{
    if (!X509_STORE_add_lookup(store, method)) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't add lookup to X509_STORE. X509_STORE_add_lookup(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::setCertificateDepth
/// \param store - OpenSSL X509_STORE.
/// \param depth - That is the maximum number of untrusted CA certificates that can appear in a chain. Example: 0.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::setDepth(X509_STORE* store, const int& depth)
{
    if (!X509_STORE_set_depth(store, depth)) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't set depth for X509_STORE. X509_STORE_set_depth(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::setFlag
/// \param store - OpenSSL X509_STORE.
/// \param flag - The verification flags consists of zero or more of the following flags ored together. Example: X509_V_FLAG_CRL_CHECK.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::setFlag(X509_STORE* store, const unsigned long& flag)
{
    if (!X509_STORE_set_flags(store, flag)) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't set flag for X509_STORE. X509_STORE_set_flags(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::setFlag
/// \param store - OpenSSL X509_STORE.
/// \param purpose - Verification purpose in param to purpose. Example: X509_PURPOSE_ANY.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::setPurpose(X509_STORE* store, const int& purpose)
{
    if (!X509_STORE_set_purpose(store, purpose)) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't set purpose for X509_STORE. X509_STORE_set_purpose(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::setTrust
/// \param store - OpenSSL X509_STORE.
/// \param trust - Trust Level. Example: X509_TRUST_SSL_SERVER.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::setTrust(X509_STORE* store, const int& trust)
{
    if (!X509_STORE_set_trust(store, trust)) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't set trust for X509_STORE. X509_STORE_set_trust(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::setDefaultPaths
/// \param store - OpenSSL X509_STORE.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::setDefaultPaths(X509_STORE* store)
{
    if (!X509_STORE_set_default_paths(store)) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't set default paths for X509_STORE. X509_STORE_set_default_paths(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::loadLocations
/// \param store - OpenSSL X509_STORE.
/// \param fileName - File name. Example: "caCertificate.pem".
/// \param dirPath - Path to file. Example: "path/To/File".
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::loadLocations(X509_STORE* store, const QByteArray& fileName, const QByteArray& dirPath)
{
    if (!X509_STORE_load_locations(store, fileName, dirPath)) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't load locations for X509_STORE. X509_STORE_load_locations(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::loadLocations
/// \param store - OpenSSL X509_STORE.
/// \param file - Qt QFile that will be loaded.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::loadLocations(X509_STORE* store, const QFile& file)
{
    /* Initialize QFileInfo to read information about file */
    QFileInfo info(file);

    if (!X509_STORE_load_locations(store, info.fileName().toLocal8Bit(), info.absoluteDir().path().toLocal8Bit())) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't load locations for X509_STORE. X509_STORE_load_locations(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}

///
/// \brief QSimpleCrypto::QX509Store::loadLocations
/// \param store - OpenSSL X509_STORE.
/// \param fileInfo - Qt QFileInfo.
/// \return Returns 'true' on success and 'false', if error happened.
///
bool QSimpleCrypto::QX509Store::loadLocations(X509_STORE* store, const QFileInfo& fileInfo)
{
    if (!X509_STORE_load_locations(store, fileInfo.fileName().toLocal8Bit(), fileInfo.absoluteDir().path().toLocal8Bit())) {
        QSimpleCrypto::QX509Store::error.setError(1, "Couldn't load locations for X509_STORE. X509_STORE_load_locations(). Error: " + QByteArray(ERR_error_string(ERR_get_error(), nullptr)));
        return false;
    }

    return true;
}
