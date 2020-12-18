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

#include "sshcryptofacility_p.h"

#include "sshbotanconversions_p.h"
#include "sshcapabilities_p.h"
#include "sshexception_p.h"
#include "sshkeyexchange_p.h"
#include "sshkeypasswordretriever_p.h"
#include "sshlogging_p.h"
#include "sshpacket_p.h"

#include <botan/botan.h>

#include <QDebug>
#include <QList>

#include <string>

using namespace Botan;

namespace QSsh {
namespace Internal {

SshAbstractCryptoFacility::SshAbstractCryptoFacility()
    : m_cipherBlockSize(0), m_macLength(0)
{
}

SshAbstractCryptoFacility::~SshAbstractCryptoFacility() {}

void SshAbstractCryptoFacility::clearKeys()
{
    m_cipherBlockSize = 0;
    m_macLength = 0;
    m_sessionId.clear();
    m_pipe.reset(0);
    m_hMac.reset(0);
}

SshAbstractCryptoFacility::Mode SshAbstractCryptoFacility::getMode(const QByteArray &algoName)
{
    if (algoName.endsWith("-ctr"))
        return CtrMode;
    if (algoName.endsWith("-cbc"))
        return CbcMode;
    throw SshClientException(SshInternalError, SSH_TR("Unexpected cipher \"%1\"")
                             .arg(QString::fromLatin1(algoName)));
}

void SshAbstractCryptoFacility::recreateKeys(const SshKeyExchange &kex)
{
    checkInvariant();

    if (m_sessionId.isEmpty())
        m_sessionId = kex.h();
    Algorithm_Factory &af = global_state().algorithm_factory();
    const QByteArray &rfcCryptAlgoName = cryptAlgoName(kex);
    BlockCipher * const cipher
            = af.prototype_block_cipher(botanCryptAlgoName(rfcCryptAlgoName))->clone();

    m_cipherBlockSize = static_cast<quint32>(cipher->block_size());
    const QByteArray ivData = generateHash(kex, ivChar(), m_cipherBlockSize);
    const InitializationVector iv(convertByteArray(ivData), m_cipherBlockSize);

    const quint32 keySize = static_cast<quint32>(cipher->key_spec().maximum_keylength());
    const QByteArray cryptKeyData = generateHash(kex, keyChar(), keySize);
    SymmetricKey cryptKey(convertByteArray(cryptKeyData), keySize);
    Keyed_Filter * const cipherMode
            = makeCipherMode(cipher, getMode(rfcCryptAlgoName), iv, cryptKey);
    m_pipe.reset(new Pipe(cipherMode));

    m_macLength = botanHMacKeyLen(hMacAlgoName(kex));
    const QByteArray hMacKeyData = generateHash(kex, macChar(), macLength());
    SymmetricKey hMacKey(convertByteArray(hMacKeyData), macLength());
    const HashFunction * const hMacProto
        = af.prototype_hash_function(botanHMacAlgoName(hMacAlgoName(kex)));
    m_hMac.reset(new HMAC(hMacProto->clone()));
    m_hMac->set_key(hMacKey);
}

void SshAbstractCryptoFacility::convert(QByteArray &data, quint32 offset,
    quint32 dataSize) const
{
    Q_ASSERT(offset + dataSize <= static_cast<quint32>(data.size()));
    checkInvariant();

    // Session id empty => No key exchange has happened yet.
    if (dataSize == 0 || m_sessionId.isEmpty())
        return;

    if (dataSize % cipherBlockSize() != 0) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid packet size");
    }
    m_pipe->process_msg(reinterpret_cast<const byte *>(data.constData()) + offset,
        dataSize);
     // Can't use Pipe::LAST_MESSAGE because of a VC bug.
    quint32 bytesRead = static_cast<quint32>(m_pipe->read(
          reinterpret_cast<byte *>(data.data()) + offset, dataSize, m_pipe->message_count() - 1));
    if (bytesRead != dataSize) {
        throw SshClientException(SshInternalError,
                QLatin1String("Internal error: Botan::Pipe::read() returned unexpected value"));
    }
}

Keyed_Filter *SshAbstractCryptoFacility::makeCtrCipherMode(BlockCipher *cipher,
        const InitializationVector &iv, const SymmetricKey &key)
{
    StreamCipher_Filter * const filter = new StreamCipher_Filter(new CTR_BE(cipher));
    filter->set_key(key);
    filter->set_iv(iv);
    return filter;
}

QByteArray SshAbstractCryptoFacility::generateMac(const QByteArray &data,
    quint32 dataSize) const
{
    return m_sessionId.isEmpty()
        ? QByteArray()
        : convertByteArray(m_hMac->process(reinterpret_cast<const byte *>(data.constData()),
              dataSize));
}

QByteArray SshAbstractCryptoFacility::generateHash(const SshKeyExchange &kex,
    char c, quint32 length)
{
    const QByteArray &k = kex.k();
    const QByteArray &h = kex.h();
    QByteArray data(k);
    data.append(h).append(c).append(m_sessionId);
    SecureVector<byte> key
        = kex.hash()->process(convertByteArray(data), data.size());
    while (key.size() < length) {
        SecureVector<byte> tmpKey;
        tmpKey += SecureVector<byte>(convertByteArray(k), k.size());
        tmpKey += SecureVector<byte>(convertByteArray(h), h.size());
        tmpKey += key;
        key += kex.hash()->process(tmpKey);
    }
    return QByteArray(reinterpret_cast<const char *>(key.begin()), length);
}

void SshAbstractCryptoFacility::checkInvariant() const
{
    Q_ASSERT(m_sessionId.isEmpty() == !m_pipe);
}


const QByteArray SshEncryptionFacility::PrivKeyFileStartLineRsa("-----BEGIN RSA PRIVATE KEY-----");
const QByteArray SshEncryptionFacility::PrivKeyFileStartLineDsa("-----BEGIN DSA PRIVATE KEY-----");
const QByteArray SshEncryptionFacility::PrivKeyFileEndLineRsa("-----END RSA PRIVATE KEY-----");
const QByteArray SshEncryptionFacility::PrivKeyFileEndLineDsa("-----END DSA PRIVATE KEY-----");
const QByteArray SshEncryptionFacility::PrivKeyFileStartLineEcdsa("-----BEGIN EC PRIVATE KEY-----");
const QByteArray SshEncryptionFacility::PrivKeyFileEndLineEcdsa("-----END EC PRIVATE KEY-----");

QByteArray SshEncryptionFacility::cryptAlgoName(const SshKeyExchange &kex) const
{
    return kex.encryptionAlgo();
}

QByteArray SshEncryptionFacility::hMacAlgoName(const SshKeyExchange &kex) const
{
    return kex.hMacAlgoClientToServer();
}

Keyed_Filter *SshEncryptionFacility::makeCipherMode(BlockCipher *cipher, Mode mode,
        const InitializationVector &iv, const SymmetricKey &key)
{
    switch (mode) {
    case CbcMode:
        return new CBC_Encryption(cipher, new Null_Padding, key, iv);
    case CtrMode:
        return makeCtrCipherMode(cipher, iv, key);
    }
    return 0; // For dumb compilers.
}

void SshEncryptionFacility::encrypt(QByteArray &data) const
{
    convert(data, 0, data.size());
}

void SshEncryptionFacility::createAuthenticationKey(const QByteArray &privKeyFileContents)
{
    if (privKeyFileContents == m_cachedPrivKeyContents)
        return;

    m_authKeyAlgoName.clear();
    qCDebug(sshLog, "%s: Key not cached, reading", Q_FUNC_INFO);
    QList<BigInt> pubKeyParams;
    QList<BigInt> allKeyParams;
    QString error1;
    QString error2;
    if (!createAuthenticationKeyFromPKCS8(privKeyFileContents, pubKeyParams, allKeyParams, error1)
            && !createAuthenticationKeyFromOpenSSL(privKeyFileContents, pubKeyParams, allKeyParams,
                error2)) {
        qCDebug(sshLog, "%s: %s\n\t%s\n", Q_FUNC_INFO, qPrintable(error1), qPrintable(error2));
        throw SshClientException(SshKeyFileError, SSH_TR("Decoding of private key file failed: "
            "Format not understood."));
    }

    foreach (const BigInt &b, allKeyParams) {
        if (b.is_zero()) {
            throw SshClientException(SshKeyFileError,
                SSH_TR("Decoding of private key file failed: Invalid zero parameter."));
        }
    }

    m_authPubKeyBlob = AbstractSshPacket::encodeString(m_authKeyAlgoName);
    auto * const ecdsaKey = dynamic_cast<ECDSA_PrivateKey *>(m_authKey.data());
    if (ecdsaKey) {
        m_authPubKeyBlob += AbstractSshPacket::encodeString(m_authKeyAlgoName.mid(11)); // Without "ecdsa-sha2-" prefix.
        m_authPubKeyBlob += AbstractSshPacket::encodeString(
                    convertByteArray(EC2OSP(ecdsaKey->public_point(), PointGFp::UNCOMPRESSED)));
    } else {
        foreach (const BigInt &b, pubKeyParams)
            m_authPubKeyBlob += AbstractSshPacket::encodeMpInt(b);
    }
    m_cachedPrivKeyContents = privKeyFileContents;
}

bool SshEncryptionFacility::createAuthenticationKeyFromPKCS8(const QByteArray &privKeyFileContents,
    QList<BigInt> &pubKeyParams, QList<BigInt> &allKeyParams, QString &error)
{
    try {
        Pipe pipe;
        pipe.process_msg(convertByteArray(privKeyFileContents), privKeyFileContents.size());
        m_authKey.reset(PKCS8::load_key(pipe, m_rng, SshKeyPasswordRetriever()));
        if (auto * const dsaKey = dynamic_cast<DSA_PrivateKey *>(m_authKey.data())) {
            m_authKeyAlgoName = SshCapabilities::PubKeyDss;
            pubKeyParams << dsaKey->group_p() << dsaKey->group_q()
                         << dsaKey->group_g() << dsaKey->get_y();
            allKeyParams << pubKeyParams << dsaKey->get_x();
        } else if (auto * const rsaKey = dynamic_cast<RSA_PrivateKey *>(m_authKey.data())) {
            m_authKeyAlgoName = SshCapabilities::PubKeyRsa;
            pubKeyParams << rsaKey->get_e() << rsaKey->get_n();
            allKeyParams << pubKeyParams << rsaKey->get_p() << rsaKey->get_q()
                         << rsaKey->get_d();
        } else if (auto * const ecdsaKey = dynamic_cast<ECDSA_PrivateKey *>(m_authKey.data())) {
            const BigInt value = ecdsaKey->private_value();
            m_authKeyAlgoName = SshCapabilities::ecdsaPubKeyAlgoForKeyWidth(
                        static_cast<int>(value.bytes()));
            pubKeyParams << ecdsaKey->public_point().get_affine_x()
                         << ecdsaKey->public_point().get_affine_y();
            allKeyParams << pubKeyParams << value;
        } else {
            qCWarning(sshLog, "%s: Unexpected code flow, expected success or exception.",
                      Q_FUNC_INFO);
            return false;
        }
    } catch (const std::exception &ex) {
        error = QLatin1String(ex.what());
        return false;
    }

    return true;
}

bool SshEncryptionFacility::createAuthenticationKeyFromOpenSSL(const QByteArray &privKeyFileContents,
    QList<BigInt> &pubKeyParams, QList<BigInt> &allKeyParams, QString &error)
{
    try {
        bool syntaxOk = true;
        QList<QByteArray> lines = privKeyFileContents.split('\n');
        while (lines.last().isEmpty())
            lines.removeLast();
        if (lines.count() < 3) {
            syntaxOk = false;
        } else if (lines.first() == PrivKeyFileStartLineRsa) {
            if (lines.last() != PrivKeyFileEndLineRsa)
                syntaxOk = false;
            else
                m_authKeyAlgoName = SshCapabilities::PubKeyRsa;
        } else if (lines.first() == PrivKeyFileStartLineDsa) {
            if (lines.last() != PrivKeyFileEndLineDsa)
                syntaxOk = false;
            else
                m_authKeyAlgoName = SshCapabilities::PubKeyDss;
        } else if (lines.first() == PrivKeyFileStartLineEcdsa) {
            if (lines.last() != PrivKeyFileEndLineEcdsa)
                syntaxOk = false;
            // m_authKeyAlgoName set below, as we don't know the size yet.
        } else {
            syntaxOk = false;
        }
        if (!syntaxOk) {
            error = SSH_TR("Unexpected format.");
            return false;
        }

        QByteArray privateKeyBlob;
        for (int i = 1; i < lines.size() - 1; ++i)
            privateKeyBlob += lines.at(i);
        privateKeyBlob = QByteArray::fromBase64(privateKeyBlob);

        BER_Decoder decoder(convertByteArray(privateKeyBlob), privateKeyBlob.size());
        BER_Decoder sequence = decoder.start_cons(SEQUENCE);
        size_t version;
        sequence.decode (version);
        const size_t expectedVersion = m_authKeyAlgoName.isEmpty() ? 1 : 0;
        if (version != expectedVersion) {
            error = SSH_TR("Key encoding has version %1, expected %2.")
                    .arg(version).arg(expectedVersion);
            return false;
        }

        if (m_authKeyAlgoName == SshCapabilities::PubKeyDss) {
            BigInt p, q, g, y, x;
            sequence.decode (p).decode (q).decode (g).decode (y).decode (x);
            DSA_PrivateKey * const dsaKey = new DSA_PrivateKey(m_rng, DL_Group(p, q, g), x);
            m_authKey.reset(dsaKey);
            pubKeyParams << p << q << g << y;
            allKeyParams << pubKeyParams << x;
        } else if (m_authKeyAlgoName == SshCapabilities::PubKeyRsa) {
            BigInt p, q, e, d, n;
            sequence.decode(n).decode(e).decode(d).decode(p).decode(q);
            RSA_PrivateKey * const rsaKey = new RSA_PrivateKey(m_rng, p, q, e, d, n);
            m_authKey.reset(rsaKey);
            pubKeyParams << e << n;
            allKeyParams << pubKeyParams << p << q << d;
        } else {
            BigInt privKey;
            sequence.decode_octet_string_bigint(privKey);
            m_authKeyAlgoName = SshCapabilities::ecdsaPubKeyAlgoForKeyWidth(
                        static_cast<int>(privKey.bytes()));
            const EC_Group group(SshCapabilities::oid(m_authKeyAlgoName));
            auto * const key = new ECDSA_PrivateKey(m_rng, group, privKey);
            m_authKey.reset(key);
            pubKeyParams << key->public_point().get_affine_x()
                         << key->public_point().get_affine_y();
            allKeyParams << pubKeyParams << privKey;
        }

        sequence.discard_remaining();
        sequence.verify_end();
    } catch (const std::exception &ex) {
        error = QLatin1String(ex.what());
        return false;
    }
    return true;
}

QByteArray SshEncryptionFacility::authenticationAlgorithmName() const
{
    Q_ASSERT(m_authKey);
    return m_authKeyAlgoName;
}

QByteArray SshEncryptionFacility::authenticationKeySignature(const QByteArray &data) const
{
    Q_ASSERT(m_authKey);

    QScopedPointer<PK_Signer> signer(new PK_Signer(*m_authKey,
        botanEmsaAlgoName(m_authKeyAlgoName)));
    QByteArray dataToSign = AbstractSshPacket::encodeString(sessionId()) + data;
    QByteArray signature
        = convertByteArray(signer->sign_message(convertByteArray(dataToSign),
              dataToSign.size(), m_rng));
    if (m_authKeyAlgoName.startsWith(SshCapabilities::PubKeyEcdsaPrefix)) {
        // The Botan output is not quite in the format that SSH defines.
        const int halfSize = signature.count() / 2;
        const BigInt r = BigInt::decode(convertByteArray(signature), halfSize);
        const BigInt s = BigInt::decode(convertByteArray(signature.mid(halfSize)), halfSize);
        signature = AbstractSshPacket::encodeMpInt(r) + AbstractSshPacket::encodeMpInt(s);
    }
    return AbstractSshPacket::encodeString(m_authKeyAlgoName)
        + AbstractSshPacket::encodeString(signature);
}

QByteArray SshEncryptionFacility::getRandomNumbers(int count) const
{
    QByteArray data;
    data.resize(count);
    m_rng.randomize(convertByteArray(data), count);
    return data;
}

SshEncryptionFacility::~SshEncryptionFacility() {}


QByteArray SshDecryptionFacility::cryptAlgoName(const SshKeyExchange &kex) const
{
    return kex.decryptionAlgo();
}

QByteArray SshDecryptionFacility::hMacAlgoName(const SshKeyExchange &kex) const
{
    return kex.hMacAlgoServerToClient();
}

Keyed_Filter *SshDecryptionFacility::makeCipherMode(BlockCipher *cipher, Mode mode, const InitializationVector &iv,
    const SymmetricKey &key)
{
    switch (mode) {
    case CbcMode:
        return new CBC_Decryption(cipher, new Null_Padding, key, iv);
    case CtrMode:
        return makeCtrCipherMode(cipher, iv, key);
    }
    return 0; // For dumb compilers.
}

void SshDecryptionFacility::decrypt(QByteArray &data, quint32 offset,
    quint32 dataSize) const
{
    convert(data, offset, dataSize);
    qCDebug(sshLog, "Decrypted data:");
    const char * const start = data.constData() + offset;
    const char * const end = start + dataSize;
    for (const char *c = start; c < end; ++c)
        qCDebug(sshLog, ) << "'" << *c << "' (0x" << (static_cast<int>(*c) & 0xff) << ")";
}

} // namespace Internal
} // namespace QSsh
