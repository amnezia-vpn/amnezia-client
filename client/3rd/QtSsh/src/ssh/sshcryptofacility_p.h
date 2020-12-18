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

#include <botan/botan.h>

#include <QByteArray>
#include <QScopedPointer>

namespace QSsh {
namespace Internal {

class SshKeyExchange;

class SshAbstractCryptoFacility
{
public:
    virtual ~SshAbstractCryptoFacility();

    void clearKeys();
    void recreateKeys(const SshKeyExchange &kex);
    QByteArray generateMac(const QByteArray &data, quint32 dataSize) const;
    quint32 cipherBlockSize() const { return m_cipherBlockSize; }
    quint32 macLength() const { return m_macLength; }

protected:
    enum Mode { CbcMode, CtrMode };

    SshAbstractCryptoFacility();
    void convert(QByteArray &data, quint32 offset, quint32 dataSize) const;
    QByteArray sessionId() const { return m_sessionId; }
    Botan::Keyed_Filter *makeCtrCipherMode(Botan::BlockCipher *cipher,
        const Botan::InitializationVector &iv, const Botan::SymmetricKey &key);

private:
    SshAbstractCryptoFacility(const SshAbstractCryptoFacility &);
    SshAbstractCryptoFacility &operator=(const SshAbstractCryptoFacility &);

    virtual QByteArray cryptAlgoName(const SshKeyExchange &kex) const = 0;
    virtual QByteArray hMacAlgoName(const SshKeyExchange &kex) const = 0;
    virtual Botan::Keyed_Filter *makeCipherMode(Botan::BlockCipher *cipher,
        Mode mode, const Botan::InitializationVector &iv, const Botan::SymmetricKey &key) = 0;
    virtual char ivChar() const = 0;
    virtual char keyChar() const = 0;
    virtual char macChar() const = 0;

    QByteArray generateHash(const SshKeyExchange &kex, char c, quint32 length);
    void checkInvariant() const;
    static Mode getMode(const QByteArray &algoName);

    QByteArray m_sessionId;
    QScopedPointer<Botan::Pipe> m_pipe;
    QScopedPointer<Botan::HMAC> m_hMac;
    quint32 m_cipherBlockSize;
    quint32 m_macLength;
};

class SshEncryptionFacility : public SshAbstractCryptoFacility
{
public:
    void encrypt(QByteArray &data) const;

    void createAuthenticationKey(const QByteArray &privKeyFileContents);
    QByteArray authenticationAlgorithmName() const;
    QByteArray authenticationPublicKey() const { return m_authPubKeyBlob; }
    QByteArray authenticationKeySignature(const QByteArray &data) const;
    QByteArray getRandomNumbers(int count) const;

    ~SshEncryptionFacility();

private:
    virtual QByteArray cryptAlgoName(const SshKeyExchange &kex) const;
    virtual QByteArray hMacAlgoName(const SshKeyExchange &kex) const;
    virtual Botan::Keyed_Filter *makeCipherMode(Botan::BlockCipher *cipher,
        Mode mode, const Botan::InitializationVector &iv, const Botan::SymmetricKey &key);
    virtual char ivChar() const { return 'A'; }
    virtual char keyChar() const { return 'C'; }
    virtual char macChar() const { return 'E'; }

    bool createAuthenticationKeyFromPKCS8(const QByteArray &privKeyFileContents,
        QList<Botan::BigInt> &pubKeyParams, QList<Botan::BigInt> &allKeyParams, QString &error);
    bool createAuthenticationKeyFromOpenSSL(const QByteArray &privKeyFileContents,
        QList<Botan::BigInt> &pubKeyParams, QList<Botan::BigInt> &allKeyParams, QString &error);

    static const QByteArray PrivKeyFileStartLineRsa;
    static const QByteArray PrivKeyFileStartLineDsa;
    static const QByteArray PrivKeyFileEndLineRsa;
    static const QByteArray PrivKeyFileEndLineDsa;
    static const QByteArray PrivKeyFileStartLineEcdsa;
    static const QByteArray PrivKeyFileEndLineEcdsa;

    QByteArray m_authKeyAlgoName;
    QByteArray m_authPubKeyBlob;
    QByteArray m_cachedPrivKeyContents;
    QScopedPointer<Botan::Private_Key> m_authKey;
    mutable Botan::AutoSeeded_RNG m_rng;
};

class SshDecryptionFacility : public SshAbstractCryptoFacility
{
public:
    void decrypt(QByteArray &data, quint32 offset, quint32 dataSize) const;

private:
    virtual QByteArray cryptAlgoName(const SshKeyExchange &kex) const;
    virtual QByteArray hMacAlgoName(const SshKeyExchange &kex) const;
    virtual Botan::Keyed_Filter *makeCipherMode(Botan::BlockCipher *cipher,
        Mode mode, const Botan::InitializationVector &iv, const Botan::SymmetricKey &key);
    virtual char ivChar() const { return 'B'; }
    virtual char keyChar() const { return 'D'; }
    virtual char macChar() const { return 'F'; }
};

} // namespace Internal
} // namespace QSsh
