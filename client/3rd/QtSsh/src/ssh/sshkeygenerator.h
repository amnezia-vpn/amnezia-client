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
