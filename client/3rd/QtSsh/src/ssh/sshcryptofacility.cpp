/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "sshcryptofacility_p.h"

#include "opensshkeyfilereader_p.h"
#include "sshbotanconversions_p.h"
#include "sshcapabilities_p.h"
#include "sshexception_p.h"
#include "sshkeyexchange_p.h"
#include "sshkeypasswordretriever_p.h"
#include "sshpacket_p.h"
#include "sshlogging_p.h"

#include <botan/block_cipher.h>
#include <botan/hash.h>
#include <botan/pkcs8.h>
#include <botan/dsa.h>
#include <botan/rsa.h>
#include <botan/ber_dec.h>
#include <botan/pubkey.h>
#include <botan/filters.h>
#include <botan/ecdsa.h>

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
    m_pipe.reset(nullptr);
    m_hMac.reset(nullptr);
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
   const QByteArray &rfcCryptAlgoName = cryptAlgoName(kex);

   { // Don't know how else to get this with the new botan API
       std::unique_ptr<BlockCipher> cipher
               = BlockCipher::create_or_throw(botanCryptAlgoName(rfcCryptAlgoName));
       m_cipherBlockSize = static_cast<quint32>(cipher->block_size());
   }
    const QByteArray ivData = generateHash(kex, ivChar(), m_cipherBlockSize);
    const InitializationVector iv(convertByteArray(ivData), m_cipherBlockSize);

    Keyed_Filter * const cipherMode
            = makeCipherMode(botanCipherAlgoName(rfcCryptAlgoName), getMode(rfcCryptAlgoName));

    const quint32 keySize = static_cast<quint32>(cipherMode->key_spec().maximum_keylength());
    const QByteArray cryptKeyData = generateHash(kex, keyChar(), keySize);
    SymmetricKey cryptKey(convertByteArray(cryptKeyData), keySize);

    cipherMode->set_key(cryptKey);
    cipherMode->set_iv(iv);

    m_pipe.reset(new Pipe(cipherMode));

    m_macLength = botanHMacKeyLen(hMacAlgoName(kex));
    const QByteArray hMacKeyData = generateHash(kex, macChar(), macLength());
    SymmetricKey hMacKey(convertByteArray(hMacKeyData), macLength());
    m_hMac = MessageAuthenticationCode::create_or_throw("HMAC(" + std::string(botanHMacAlgoName(hMacAlgoName(kex))) + ")");
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

Keyed_Filter *SshAbstractCryptoFacility::makeCtrCipherMode(const QByteArray &cipher)
{
    StreamCipher_Filter *filter = new StreamCipher_Filter(cipher.toStdString());
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
        secure_vector<byte> tmpKey;
        tmpKey += secure_vector<byte>(k.begin(), k.end());
        tmpKey += secure_vector<byte>(h.begin(), h.end());
        tmpKey += key;
        key += kex.hash()->process(tmpKey);
    }
    return QByteArray(reinterpret_cast<const char *>(key.data()), length);
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

Keyed_Filter *SshEncryptionFacility::makeCipherMode(const QByteArray &cipher, const Mode mode)
{
    if (mode == CtrMode) {
        return new StreamCipher_Filter(cipher.toStdString());
    }

    qWarning() << "I haven't been able to test the CBC encryption modes, so if this files file a bug at https://github.com/sandsmark/QSsh";

    Cipher_Mode_Filter *filter = new Cipher_Mode_Filter(
                Cipher_Mode::create_or_throw(cipher.toStdString(), ENCRYPTION).release()); // We have to release, otherwise clang fails to link
    return filter;
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
    OpenSshKeyFileReader openSshReader(m_rng);
    if (openSshReader.parseKey(privKeyFileContents)) {
        m_authKeyAlgoName = openSshReader.keyType();
        m_authKey.reset(openSshReader.privateKey().release());
        pubKeyParams = openSshReader.publicParameters();
        allKeyParams = openSshReader.allParameters();
    } else if (!createAuthenticationKeyFromPKCS8(privKeyFileContents, pubKeyParams, allKeyParams,
                                                 error1)
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
                    convertByteArray(ecdsaKey->public_point().encode(PointGFp::UNCOMPRESSED)));
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
        m_authKey.reset(PKCS8::load_key(pipe, m_rng, SshKeyPasswordRetriever::get_passphrase));
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
            RSA_PrivateKey * const rsaKey = new RSA_PrivateKey(p, q, e, d, n);
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
        m_rng,
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

Keyed_Filter *SshDecryptionFacility::makeCipherMode(const QByteArray &cipher, const Mode mode)
{
    if (mode == CtrMode) {
        return new StreamCipher_Filter(cipher.toStdString());
    }

    qWarning() << "I haven't been able to test the CBC decryption modes, so if this files file a bug at https://github.com/sandsmark/QSsh";

    Cipher_Mode_Filter *filter = new Cipher_Mode_Filter(
                Cipher_Mode::create_or_throw(cipher.toStdString(), DECRYPTION).release()); // We have to release, otherwise clang fails to link
    return filter;
}

void SshDecryptionFacility::decrypt(QByteArray &data, quint32 offset,
    quint32 dataSize) const
{
    convert(data, offset, dataSize);
    qCDebug(sshLog, "Decrypted data:");
    const char * const start = data.constData() + offset;
    const char * const end = start + dataSize;
    for (const char *c = start; c < end; ++c)
        qCDebug(sshLog) << "'" << *c << "' (0x" << (static_cast<int>(*c) & 0xff) << ")";
}

} // namespace Internal
} // namespace QSsh
