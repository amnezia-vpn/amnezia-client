/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "opensshkeyfilereader_p.h"

#include "sshcapabilities_p.h"
#include "ssherrors.h"
#include "sshexception_p.h"
#include "sshlogging_p.h"
#include "sshpacketparser_p.h"
#include "ssh_global.h"

#include <botan/dl_group.h>
#include <botan/dsa.h>
#include <botan/ecdsa.h>
#include <botan/rsa.h>

#include <memory>

namespace QSsh {
namespace Internal {

using namespace Botan;

bool OpenSshKeyFileReader::parseKey(const QByteArray &privKeyFileContents)
{
    static const QByteArray magicPrefix = "-----BEGIN OPENSSH PRIVATE KEY-----\n";
    static const QByteArray magicSuffix = "-----END OPENSSH PRIVATE KEY-----\n";
    if (!privKeyFileContents.startsWith(magicPrefix)) {
        qCDebug(sshLog) << "not an OpenSSH key file: prefix does not match";
        return false;
    }
    if (!privKeyFileContents.endsWith(magicSuffix))
        throwException(SSH_TR("Unexpected end-of-file marker."));
    const QByteArray payload = QByteArray::fromBase64
            (privKeyFileContents.mid(magicPrefix.size(), privKeyFileContents.size()
                                     - magicPrefix.size() - magicSuffix.size()));
    doParse(payload);
    return true;
}

std::unique_ptr<Private_Key> OpenSshKeyFileReader::privateKey() const
{
    if (m_keyType == SshCapabilities::PubKeyRsa) {
        QSSH_ASSERT_AND_RETURN_VALUE(m_parameters.size() == 5, nullptr);
        const BigInt &e = m_parameters.at(0);
        const BigInt &n = m_parameters.at(1);
        const BigInt &p = m_parameters.at(2);
        const BigInt &q = m_parameters.at(3);
        const BigInt &d = m_parameters.at(4);
        return std::make_unique<RSA_PrivateKey>(p, q, e, d, n);
    } else if (m_keyType == SshCapabilities::PubKeyDss) {
        QSSH_ASSERT_AND_RETURN_VALUE(m_parameters.size() == 5, nullptr);
        const BigInt &p = m_parameters.at(0);
        const BigInt &q = m_parameters.at(1);
        const BigInt &g = m_parameters.at(2);
        const BigInt &x = m_parameters.at(4);
        return std::make_unique<DSA_PrivateKey>(m_rng, DL_Group(p, q, g), x);
    } else if (m_keyType.startsWith(SshCapabilities::PubKeyEcdsaPrefix)) {
        QSSH_ASSERT_AND_RETURN_VALUE(m_parameters.size() == 1, nullptr);
        const BigInt &value = m_parameters.first();
        const EC_Group group(SshCapabilities::oid(m_keyType));
        return std::make_unique<ECDSA_PrivateKey>(m_rng, group, value);
    }
    QSSH_ASSERT_AND_RETURN_VALUE(false, nullptr);
}

QList<BigInt> OpenSshKeyFileReader::publicParameters() const
{
    if (m_keyType == SshCapabilities::PubKeyRsa)
        return m_parameters.mid(0, 2);
    if (m_keyType == SshCapabilities::PubKeyDss)
        return m_parameters.mid(0, 4);
    if (m_keyType.startsWith(SshCapabilities::PubKeyEcdsaPrefix))
        return QList<BigInt>();
    QSSH_ASSERT_AND_RETURN_VALUE(false, QList<BigInt>());
}

void OpenSshKeyFileReader::doParse(const QByteArray &payload)
{
    // See PROTOCOL.key in OpenSSH sources.
    static const QByteArray magicString = "openssh-key-v1";
    if (!payload.startsWith(magicString))
        throwException(SSH_TR("Unexpected magic string."));
    try {
        quint32 offset = magicString.size() + 1; // null byte
        m_cipherName = SshPacketParser::asString(payload, &offset);
        qCDebug(sshLog) << "cipher:" << m_cipherName;
        m_kdf = SshPacketParser::asString(payload, &offset);
        qCDebug(sshLog) << "kdf:" << m_kdf;
        parseKdfOptions(SshPacketParser::asString(payload, &offset));
        const quint32 keyCount = SshPacketParser::asUint32(payload, &offset);
        if (keyCount != 1) {
            qCWarning(sshLog) << "more than one key found in OpenSSH private key file, ignoring "
                                 "all but the first one";
        }
        for (quint32 i = 0; i < keyCount; ++i) // Skip the public key blob(s).
            SshPacketParser::asString(payload, &offset);
        m_privateKeyList = SshPacketParser::asString(payload, &offset);
        decryptPrivateKeyList();
        parsePrivateKeyList();
    } catch (const SshPacketParseException &) {
        throwException(SSH_TR("Parse error."));
    } catch (const Exception &e) {
        throwException(QLatin1String(e.what()));
    }
}

void OpenSshKeyFileReader::parseKdfOptions(const QByteArray &kdfOptions)
{
    if (m_cipherName == "none")
        return;
    quint32 offset = 0;
    m_salt = SshPacketParser::asString(kdfOptions, &offset);
    if (m_salt.size() != 16)
        throwException(SSH_TR("Invalid salt size %1.").arg(m_salt.size()));
    m_rounds = SshPacketParser::asUint32(kdfOptions, &offset);
    qCDebug(sshLog) << "salt:" << m_salt.toHex();
    qCDebug(sshLog) << "rounds:" << m_rounds;
}

void OpenSshKeyFileReader::decryptPrivateKeyList()
{
    if (m_cipherName == "none")
        return;
    if (m_kdf != "bcrypt") {
        throwException(SSH_TR("Unexpected key derivation function '%1'.")
                       .arg(QLatin1String(m_kdf)));
    }

    // OpenSSH uses a proprietary algorithm for the key derivation. We'd basically have to
    // copy the code.
    // TODO: If the lower-level operations (hashing primitives, blowfish stuff) can be taken
    //       over by Botan, that might be feasible. Investigate.
    throwException(SSH_TR("Encrypted keys are currently not supported in this format."));
}

void OpenSshKeyFileReader::parsePrivateKeyList()
{
    quint32 offset = 0;
    const quint32 checkInt1 = SshPacketParser::asUint32(m_privateKeyList, &offset);
    const quint32 checkInt2 = SshPacketParser::asUint32(m_privateKeyList, &offset);
    if (checkInt1 != checkInt2)
        throwException(SSH_TR("Verification failed."));
    m_keyType = SshPacketParser::asString(m_privateKeyList, &offset);
    qCDebug(sshLog) << "key type:" << m_keyType;
    if (m_keyType == SshCapabilities::PubKeyRsa) {
        const BigInt n = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        const BigInt e = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        const BigInt d = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        SshPacketParser::asBigInt(m_privateKeyList, &offset); // iqmp
        const BigInt p = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        const BigInt q = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        m_parameters = QList<BigInt>{e, n, p, q, d};
    } else if (m_keyType == SshCapabilities::PubKeyDss) {
        const BigInt p = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        const BigInt q = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        const BigInt g = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        const BigInt y = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        const BigInt x = SshPacketParser::asBigInt(m_privateKeyList, &offset);
        m_parameters = QList<BigInt>{p, q, g, y, x};
    } else if (m_keyType.startsWith(SshCapabilities::PubKeyEcdsaPrefix)) {
        SshPacketParser::asString(m_privateKeyList, &offset); // name
        SshPacketParser::asString(m_privateKeyList, &offset); // pubkey representation
        m_parameters = {SshPacketParser::asBigInt(m_privateKeyList, &offset)};
    } else {
        throwException(SSH_TR("Private key type '%1' is not supported.")
                       .arg(QString::fromLatin1(m_keyType)));
    }
    const QByteArray comment = SshPacketParser::asString(m_privateKeyList, &offset);
    qCDebug(sshLog) << "comment:" << comment;
}

void OpenSshKeyFileReader::throwException(const QString &reason)
{
    throw SshClientException(SshKeyFileError,
                             SSH_TR("Processing OpenSSH private key file failed: %1").arg(reason));
}

} // namespace Internal
} // namespace QSsh
