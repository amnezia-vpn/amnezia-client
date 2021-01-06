/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "sshcapabilities_p.h"
#include "sshexception_p.h"

#include <botan/botan.h>

namespace QSsh {
namespace Internal {

inline const Botan::byte *convertByteArray(const QByteArray &a)
{
    return reinterpret_cast<const Botan::byte *>(a.constData());
}

inline Botan::byte *convertByteArray(QByteArray &a)
{
    return reinterpret_cast<Botan::byte *>(a.data());
}

inline QByteArray convertByteArray(const Botan::SecureVector<Botan::byte> &v)
{
    return QByteArray(reinterpret_cast<const char *>(v.begin()), static_cast<int>(v.size()));
}

inline const char *botanKeyExchangeAlgoName(const QByteArray &rfcAlgoName)
{
    if (rfcAlgoName == SshCapabilities::DiffieHellmanGroup1Sha1)
        return "modp/ietf/1024";
    if (rfcAlgoName == SshCapabilities::DiffieHellmanGroup14Sha1)
        return "modp/ietf/2048";
    if (rfcAlgoName == SshCapabilities::EcdhNistp256)
        return "secp256r1";
    if (rfcAlgoName == SshCapabilities::EcdhNistp384)
        return "secp384r1";
    if (rfcAlgoName == SshCapabilities::EcdhNistp521)
        return "secp521r1";
    throw SshClientException(SshInternalError, SSH_TR("Unexpected key exchange algorithm \"%1\"")
                             .arg(QString::fromLatin1(rfcAlgoName)));
}

inline const char *botanCryptAlgoName(const QByteArray &rfcAlgoName)
{
    if (rfcAlgoName == SshCapabilities::CryptAlgoAes128Cbc
            || rfcAlgoName == SshCapabilities::CryptAlgoAes128Ctr) {
        return "AES-128";
    }
    if (rfcAlgoName == SshCapabilities::CryptAlgo3DesCbc
            || rfcAlgoName == SshCapabilities::CryptAlgo3DesCtr) {
        return "TripleDES";
    }
    if (rfcAlgoName == SshCapabilities::CryptAlgoAes192Ctr) {
        return "AES-192";
    }
    if (rfcAlgoName == SshCapabilities::CryptAlgoAes256Ctr) {
        return "AES-256";
    }
    throw SshClientException(SshInternalError, SSH_TR("Unexpected cipher \"%1\"")
                             .arg(QString::fromLatin1(rfcAlgoName)));
}

inline const char *botanEmsaAlgoName(const QByteArray &rfcAlgoName)
{
    if (rfcAlgoName == SshCapabilities::PubKeyDss)
        return "EMSA1(SHA-1)";
    if (rfcAlgoName == SshCapabilities::PubKeyRsa)
        return "EMSA3(SHA-1)";
    if (rfcAlgoName == SshCapabilities::PubKeyEcdsa256)
        return "EMSA1_BSI(SHA-256)";
    if (rfcAlgoName == SshCapabilities::PubKeyEcdsa384)
        return "EMSA1_BSI(SHA-384)";
    if (rfcAlgoName == SshCapabilities::PubKeyEcdsa521)
        return "EMSA1_BSI(SHA-512)";
    throw SshClientException(SshInternalError, SSH_TR("Unexpected host key algorithm \"%1\"")
                             .arg(QString::fromLatin1(rfcAlgoName)));
}

inline const char *botanHMacAlgoName(const QByteArray &rfcAlgoName)
{
    if (rfcAlgoName == SshCapabilities::HMacSha1)
        return "SHA-1";
    if (rfcAlgoName == SshCapabilities::HMacSha256)
        return "SHA-256";
    if (rfcAlgoName == SshCapabilities::HMacSha384)
        return "SHA-384";
    if (rfcAlgoName == SshCapabilities::HMacSha512)
        return "SHA-512";
    throw SshClientException(SshInternalError, SSH_TR("Unexpected hashing algorithm \"%1\"")
                             .arg(QString::fromLatin1(rfcAlgoName)));
}

inline quint32 botanHMacKeyLen(const QByteArray &rfcAlgoName)
{
    if (rfcAlgoName == SshCapabilities::HMacSha1)
        return 20;
    if (rfcAlgoName == SshCapabilities::HMacSha256)
        return 32;
    if (rfcAlgoName == SshCapabilities::HMacSha384)
        return 48;
    if (rfcAlgoName == SshCapabilities::HMacSha512)
        return 64;
    throw SshClientException(SshInternalError, SSH_TR("Unexpected hashing algorithm \"%1\"")
                             .arg(QString::fromLatin1(rfcAlgoName)));
}

} // namespace Internal
} // namespace QSsh
