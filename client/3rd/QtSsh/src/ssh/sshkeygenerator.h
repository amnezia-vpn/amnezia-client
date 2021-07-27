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

#ifndef SSHKEYGENERATOR_H
#define SSHKEYGENERATOR_H

#include "ssh_global.h"

#include <QCoreApplication>
#include <QSharedPointer>

namespace Botan {
    class Private_Key;
    class RandomNumberGenerator;
}

namespace QSsh {

class QSSH_EXPORT SshKeyGenerator
{
    Q_DECLARE_TR_FUNCTIONS(SshKeyGenerator)
public:
    enum KeyType { Rsa, Dsa, Ecdsa };
    enum PrivateKeyFormat { Pkcs8, OpenSsl, Mixed };
    enum EncryptionMode { DoOfferEncryption, DoNotOfferEncryption }; // Only relevant for Pkcs8 format.

    SshKeyGenerator();
    bool generateKeys(KeyType type, PrivateKeyFormat format, int keySize,
        EncryptionMode encryptionMode = DoOfferEncryption);

    QString error() const { return m_error; }
    QByteArray privateKey() const { return m_privateKey; }
    QByteArray publicKey() const { return m_publicKey; }
    KeyType type() const { return m_type; }

private:
    typedef QSharedPointer<Botan::Private_Key> KeyPtr;

    void generatePkcs8KeyStrings(const KeyPtr &key, Botan::RandomNumberGenerator &rng);
    void generatePkcs8KeyString(const KeyPtr &key, bool privateKey,
        Botan::RandomNumberGenerator &rng);
    void generateOpenSslKeyStrings(const KeyPtr &key);
    void generateOpenSslPrivateKeyString(const KeyPtr &key);
    void generateOpenSslPublicKeyString(const KeyPtr &key);
    QString getPassword() const;

    QString m_error;
    QByteArray m_publicKey;
    QByteArray m_privateKey;
    KeyType m_type;
    EncryptionMode m_encryptionMode;
};

} // namespace QSsh

#endif // SSHKEYGENERATOR_H
