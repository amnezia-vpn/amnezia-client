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

#ifndef SSHABSTRACTCRYPTOFACILITY_P_H
#define SSHABSTRACTCRYPTOFACILITY_P_H

#include <botan/filters.h>
#include <botan/block_cipher.h>
#include <botan/pipe.h>
#include <botan/bigint.h>
#include <botan/pk_keys.h>
#include <botan/auto_rng.h>

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
    QByteArray sessionId() const { return m_sessionId; }

    bool isValid() const { return m_hMac && m_pipe; } // TODO: probably more, but this stops segfaulting

protected:
    enum Mode { CbcMode, CtrMode };

    SshAbstractCryptoFacility();
    void convert(QByteArray &data, quint32 offset, quint32 dataSize) const;
    Botan::Keyed_Filter *makeCtrCipherMode(const QByteArray &cipher);

private:
    SshAbstractCryptoFacility(const SshAbstractCryptoFacility &);
    SshAbstractCryptoFacility &operator=(const SshAbstractCryptoFacility &);

    virtual QByteArray cryptAlgoName(const SshKeyExchange &kex) const = 0;
    virtual QByteArray hMacAlgoName(const SshKeyExchange &kex) const = 0;
    virtual Botan::Keyed_Filter *makeCipherMode(const QByteArray &cipher, const Mode mode) = 0;
    virtual char ivChar() const = 0;
    virtual char keyChar() const = 0;
    virtual char macChar() const = 0;

    QByteArray generateHash(const SshKeyExchange &kex, char c, quint32 length);
    void checkInvariant() const;
    static Mode getMode(const QByteArray &algoName);

    QByteArray m_sessionId;
    std::unique_ptr<Botan::Pipe> m_pipe;
    std::unique_ptr<Botan::MessageAuthenticationCode> m_hMac;
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
    QByteArray cryptAlgoName(const SshKeyExchange &kex) const override;
    QByteArray hMacAlgoName(const SshKeyExchange &kex) const override;
    Botan::Keyed_Filter *makeCipherMode(const QByteArray &cipher, const Mode mode) override;
    char ivChar() const override { return 'A'; }
    char keyChar() const override { return 'C'; }
    char macChar() const override { return 'E'; }

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
    QByteArray cryptAlgoName(const SshKeyExchange &kex) const override;
    QByteArray hMacAlgoName(const SshKeyExchange &kex) const override;
    Botan::Keyed_Filter *makeCipherMode(const QByteArray &cipher, const Mode mode) override;
    char ivChar() const override { return 'B'; }
    char keyChar() const override { return 'D'; }
    char macChar() const override { return 'F'; }
};

} // namespace Internal
} // namespace QSsh

#endif // SSHABSTRACTCRYPTOFACILITY_P_H
