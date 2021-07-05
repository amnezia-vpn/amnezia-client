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

#include "sshkeygenerator.h"

#include "sshbotanconversions_p.h"
#include "sshcapabilities_p.h"
#include "ssh_global.h"
#include "sshinit_p.h"
#include "sshpacket_p.h"

#include <botan/botan.h>

#include <QDateTime>
#include <QInputDialog>

#include <string>

namespace QSsh {

using namespace Botan;
using namespace Internal;

SshKeyGenerator::SshKeyGenerator() : m_type(Rsa)
{
    initSsh();
}

bool SshKeyGenerator::generateKeys(KeyType type, PrivateKeyFormat format, int keySize,
    EncryptionMode encryptionMode)
{
    m_type = type;
    m_encryptionMode = encryptionMode;

    try {
        AutoSeeded_RNG rng;
        KeyPtr key;
        switch (m_type) {
        case Rsa:
            key = KeyPtr(new RSA_PrivateKey(rng, keySize));
            break;
        case Dsa:
            key = KeyPtr(new DSA_PrivateKey(rng, DL_Group(rng, DL_Group::DSA_Kosherizer, keySize)));
            break;
        case Ecdsa: {
            const QByteArray algo = SshCapabilities::ecdsaPubKeyAlgoForKeyWidth(keySize / 8);
            key = KeyPtr(new ECDSA_PrivateKey(rng, EC_Group(SshCapabilities::oid(algo))));
            break;
        }
        }
        switch (format) {
        case Pkcs8:
            generatePkcs8KeyStrings(key, rng);
            break;
        case OpenSsl:
            generateOpenSslKeyStrings(key);
            break;
        case Mixed:
        default:
            generatePkcs8KeyString(key, true, rng);
            generateOpenSslPublicKeyString(key);
        }
        return true;
    } catch (const std::exception &e) {
        m_error = tr("Error generating key: %1").arg(QString::fromLatin1(e.what()));
        return false;
    }
}

void SshKeyGenerator::generatePkcs8KeyStrings(const KeyPtr &key, RandomNumberGenerator &rng)
{
    generatePkcs8KeyString(key, false, rng);
    generatePkcs8KeyString(key, true, rng);
}

void SshKeyGenerator::generatePkcs8KeyString(const KeyPtr &key, bool privateKey,
    RandomNumberGenerator &rng)
{
    Pipe pipe;
    pipe.start_msg();
    QByteArray *keyData;
    if (privateKey) {
        QString password;
        if (m_encryptionMode == DoOfferEncryption)
            password = getPassword();
        if (!password.isEmpty())
            pipe.write(PKCS8::PEM_encode(*key, rng, password.toLocal8Bit().data()));
        else
            pipe.write(PKCS8::PEM_encode(*key));
        keyData = &m_privateKey;
    } else {
        pipe.write(X509::PEM_encode(*key));
        keyData = &m_publicKey;
    }
    pipe.end_msg();
    keyData->resize(static_cast<int>(pipe.remaining(pipe.message_count() - 1)));
    pipe.read(convertByteArray(*keyData), keyData->size(),
        pipe.message_count() - 1);
}

void SshKeyGenerator::generateOpenSslKeyStrings(const KeyPtr &key)
{
    generateOpenSslPublicKeyString(key);
    generateOpenSslPrivateKeyString(key);
}

void SshKeyGenerator::generateOpenSslPublicKeyString(const KeyPtr &key)
{
    QList<BigInt> params;
    QByteArray keyId;
    QByteArray q;
    switch (m_type) {
    case Rsa: {
        const QSharedPointer<RSA_PrivateKey> rsaKey = key.dynamicCast<RSA_PrivateKey>();
        params << rsaKey->get_e() << rsaKey->get_n();
        keyId = SshCapabilities::PubKeyRsa;
        break;
    }
    case Dsa: {
        const QSharedPointer<DSA_PrivateKey> dsaKey = key.dynamicCast<DSA_PrivateKey>();
        params << dsaKey->group_p() << dsaKey->group_q() << dsaKey->group_g() << dsaKey->get_y();
        keyId = SshCapabilities::PubKeyDss;
        break;
    }
    case Ecdsa: {
        const auto ecdsaKey = key.dynamicCast<ECDSA_PrivateKey>();
        q = convertByteArray(EC2OSP(ecdsaKey->public_point(), PointGFp::UNCOMPRESSED));
        keyId = SshCapabilities::ecdsaPubKeyAlgoForKeyWidth(
                    static_cast<int>(ecdsaKey->private_value().bytes()));
        break;
    }
    }

    QByteArray publicKeyBlob = AbstractSshPacket::encodeString(keyId);
    foreach (const BigInt &b, params)
        publicKeyBlob += AbstractSshPacket::encodeMpInt(b);
    if (!q.isEmpty()) {
        publicKeyBlob += AbstractSshPacket::encodeString(keyId.mid(11)); // Without "ecdsa-sha2-" prefix.
        publicKeyBlob += AbstractSshPacket::encodeString(q);
    }
    publicKeyBlob = publicKeyBlob.toBase64();
    const QByteArray id = "QtCreator/"
        + QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8();
    m_publicKey = keyId + ' ' + publicKeyBlob + ' ' + id;
}

void SshKeyGenerator::generateOpenSslPrivateKeyString(const KeyPtr &key)
{
    QList<BigInt> params;
    const char *label = "";
    switch (m_type) {
    case Rsa: {
        const QSharedPointer<RSA_PrivateKey> rsaKey
            = key.dynamicCast<RSA_PrivateKey>();
        params << rsaKey->get_n() << rsaKey->get_e() << rsaKey->get_d() << rsaKey->get_p()
            << rsaKey->get_q();
        const BigInt dmp1 = rsaKey->get_d() % (rsaKey->get_p() - 1);
        const BigInt dmq1 = rsaKey->get_d() % (rsaKey->get_q() - 1);
        const BigInt iqmp = inverse_mod(rsaKey->get_q(), rsaKey->get_p());
        params << dmp1 << dmq1 << iqmp;
        label = "RSA PRIVATE KEY";
        break;
    }
    case Dsa: {
        const QSharedPointer<DSA_PrivateKey> dsaKey = key.dynamicCast<DSA_PrivateKey>();
        params << dsaKey->group_p() << dsaKey->group_q() << dsaKey->group_g() << dsaKey->get_y()
            << dsaKey->get_x();
        label = "DSA PRIVATE KEY";
        break;
    }
    case Ecdsa:
        params << key.dynamicCast<ECDSA_PrivateKey>()->private_value();
        label = "EC PRIVATE KEY";
        break;
    }

    DER_Encoder encoder;
    encoder.start_cons(SEQUENCE).encode(size_t(0));
    foreach (const BigInt &b, params)
        encoder.encode(b);
    encoder.end_cons();
    m_privateKey = QByteArray(PEM_Code::encode (encoder.get_contents(), label).c_str());
}

QString SshKeyGenerator::getPassword() const
{
    QInputDialog d;
    d.setInputMode(QInputDialog::TextInput);
    d.setTextEchoMode(QLineEdit::Password);
    d.setWindowTitle(tr("Password for Private Key"));
    d.setLabelText(tr("It is recommended that you secure your private key\n"
        "with a password, which you can enter below."));
    d.setOkButtonText(tr("Encrypt Key File"));
    d.setCancelButtonText(tr("Do Not Encrypt Key File"));
    int result = QDialog::Accepted;
    QString password;
    while (result == QDialog::Accepted && password.isEmpty()) {
        result = d.exec();
        password = d.textValue();
    }
    return result == QDialog::Accepted ? password : QString();
}

} // namespace QSsh
