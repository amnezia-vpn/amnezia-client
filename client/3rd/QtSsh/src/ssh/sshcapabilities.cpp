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

#include "sshcapabilities_p.h"

#include "sshexception_p.h"

#include <QCoreApplication>
#include <QString>

namespace QSsh {
namespace Internal {

namespace {
    QByteArray listAsByteArray(const QList<QByteArray> &list)
    {
        QByteArray array;
        foreach (const QByteArray &elem, list)
            array += elem + ',';
        if (!array.isEmpty())
            array.remove(array.count() - 1, 1);
        return array;
    }
} // anonymous namspace

const QByteArray SshCapabilities::DiffieHellmanGroup1Sha1("diffie-hellman-group1-sha1");
const QByteArray SshCapabilities::DiffieHellmanGroup14Sha1("diffie-hellman-group14-sha1");
const QByteArray SshCapabilities::EcdhKexNamePrefix("ecdh-sha2-nistp");
const QByteArray SshCapabilities::EcdhNistp256 = EcdhKexNamePrefix + "256";
const QByteArray SshCapabilities::EcdhNistp384 = EcdhKexNamePrefix + "384";
const QByteArray SshCapabilities::EcdhNistp521 = EcdhKexNamePrefix + "521";
const QList<QByteArray> SshCapabilities::KeyExchangeMethods = QList<QByteArray>()
        << SshCapabilities::EcdhNistp256
        << SshCapabilities::EcdhNistp384
        << SshCapabilities::EcdhNistp521
        << SshCapabilities::DiffieHellmanGroup1Sha1
        << SshCapabilities::DiffieHellmanGroup14Sha1;

const QByteArray SshCapabilities::PubKeyDss("ssh-dss");
const QByteArray SshCapabilities::PubKeyRsa("ssh-rsa");
const QByteArray SshCapabilities::PubKeyEcdsaPrefix("ecdsa-sha2-nistp");
const QByteArray SshCapabilities::PubKeyEcdsa256 = SshCapabilities::PubKeyEcdsaPrefix + "256";
const QByteArray SshCapabilities::PubKeyEcdsa384 = SshCapabilities::PubKeyEcdsaPrefix + "384";
const QByteArray SshCapabilities::PubKeyEcdsa521 = SshCapabilities::PubKeyEcdsaPrefix + "521";
const QList<QByteArray> SshCapabilities::PublicKeyAlgorithms = QList<QByteArray>()
        << SshCapabilities::PubKeyEcdsa256
        << SshCapabilities::PubKeyEcdsa384
        << SshCapabilities::PubKeyEcdsa521
        << SshCapabilities::PubKeyRsa
        << SshCapabilities::PubKeyDss;

const QByteArray SshCapabilities::CryptAlgo3DesCbc("3des-cbc");
const QByteArray SshCapabilities::CryptAlgo3DesCtr("3des-ctr");
const QByteArray SshCapabilities::CryptAlgoAes128Cbc("aes128-cbc");
const QByteArray SshCapabilities::CryptAlgoAes128Ctr("aes128-ctr");
const QByteArray SshCapabilities::CryptAlgoAes192Ctr("aes192-ctr");
const QByteArray SshCapabilities::CryptAlgoAes256Ctr("aes256-ctr");
const QList<QByteArray> SshCapabilities::EncryptionAlgorithms
    = QList<QByteArray>() << SshCapabilities::CryptAlgoAes256Ctr
                          << SshCapabilities::CryptAlgoAes192Ctr
                          << SshCapabilities::CryptAlgoAes128Ctr
                          << SshCapabilities::CryptAlgo3DesCtr
                          << SshCapabilities::CryptAlgoAes128Cbc
                          << SshCapabilities::CryptAlgo3DesCbc;

const QByteArray SshCapabilities::HMacSha1("hmac-sha1");
const QByteArray SshCapabilities::HMacSha196("hmac-sha1-96");
const QByteArray SshCapabilities::HMacSha256("hmac-sha2-256");
const QByteArray SshCapabilities::HMacSha384("hmac-sha2-384");
const QByteArray SshCapabilities::HMacSha512("hmac-sha2-512");
const QList<QByteArray> SshCapabilities::MacAlgorithms
    = QList<QByteArray>() /* << SshCapabilities::HMacSha196 */
        << SshCapabilities::HMacSha256
        << SshCapabilities::HMacSha384
        << SshCapabilities::HMacSha512
        << SshCapabilities::HMacSha1;

const QList<QByteArray> SshCapabilities::CompressionAlgorithms
    = QList<QByteArray>() << "none";

const QByteArray SshCapabilities::SshConnectionService("ssh-connection");

QList<QByteArray> SshCapabilities::commonCapabilities(const QList<QByteArray> &myCapabilities,
                                               const QList<QByteArray> &serverCapabilities)
{
    QList<QByteArray> capabilities;
    foreach (const QByteArray &myCapability, myCapabilities) {
        if (serverCapabilities.contains(myCapability))
            capabilities << myCapability;
    }

    if (!capabilities.isEmpty())
        return capabilities;

    throw SshServerException(SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
        "Server and client capabilities do not match.",
        QCoreApplication::translate("SshConnection",
            "Server and client capabilities don't match. "
            "Client list was: %1.\nServer list was %2.")
            .arg(QString::fromLocal8Bit(listAsByteArray(myCapabilities).data()))
            .arg(QString::fromLocal8Bit(listAsByteArray(serverCapabilities).data())));

}

QByteArray SshCapabilities::findBestMatch(const QList<QByteArray> &myCapabilities,
    const QList<QByteArray> &serverCapabilities)
{
    return commonCapabilities(myCapabilities, serverCapabilities).first();
}

int SshCapabilities::ecdsaIntegerWidthInBytes(const QByteArray &ecdsaAlgo)
{
    if (ecdsaAlgo == PubKeyEcdsa256)
        return 32;
    if (ecdsaAlgo == PubKeyEcdsa384)
        return 48;
    if (ecdsaAlgo == PubKeyEcdsa521)
        return 66;
    throw SshClientException(SshInternalError, SSH_TR("Unexpected ecdsa algorithm \"%1\"")
                             .arg(QString::fromLatin1(ecdsaAlgo)));
}

QByteArray SshCapabilities::ecdsaPubKeyAlgoForKeyWidth(int keyWidthInBytes)
{
    if (keyWidthInBytes <= 32)
        return PubKeyEcdsa256;
    if (keyWidthInBytes <= 48)
        return PubKeyEcdsa384;
    if (keyWidthInBytes <= 66)
        return PubKeyEcdsa521;
    throw SshClientException(SshInternalError, SSH_TR("Unexpected ecdsa key size (%1 bytes)")
                             .arg(keyWidthInBytes));
}

const char *SshCapabilities::oid(const QByteArray &ecdsaAlgo)
{
    if (ecdsaAlgo == PubKeyEcdsa256)
        return "secp256r1";
    if (ecdsaAlgo == PubKeyEcdsa384)
        return "secp384r1";
    if (ecdsaAlgo == PubKeyEcdsa521)
        return "secp521r1";
    throw SshClientException(SshInternalError, SSH_TR("Unexpected ecdsa algorithm \"%1\"")
                             .arg(QString::fromLatin1(ecdsaAlgo)));
}

} // namespace Internal
} // namespace QSsh
