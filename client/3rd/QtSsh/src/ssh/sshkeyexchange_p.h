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

#ifndef SSHKEYEXCHANGE_P_H
#define SSHKEYEXCHANGE_P_H

#include "sshconnection.h"

#include <QByteArray>
#include <QScopedPointer>

#include <memory>

namespace Botan {
class DH_PrivateKey;
class ECDH_PrivateKey;
class HashFunction;
}

namespace QSsh {
namespace Internal {

struct SshKeyExchangeInit;
class SshSendFacility;
class SshIncomingPacket;

class SshKeyExchange
{
public:
    SshKeyExchange(const SshConnectionParameters &connParams, SshSendFacility &sendFacility);
    ~SshKeyExchange();

    const QByteArray &hostKeyFingerprint() { return m_hostFingerprint; }
    void sendKexInitPacket(const QByteArray &serverId);

    // Returns true <=> the server sends a guessed package.
    bool sendDhInitPacket(const SshIncomingPacket &serverKexInit);

    void sendNewKeysPacket(const SshIncomingPacket &dhReply,
        const QByteArray &clientId);

    QByteArray k() const { return m_k; }
    QByteArray h() const { return m_h; }
    Botan::HashFunction *hash() const { return m_hash.get(); }
    QByteArray encryptionAlgo() const { return m_encryptionAlgo; }
    QByteArray decryptionAlgo() const { return m_decryptionAlgo; }
    QByteArray hMacAlgoClientToServer() const { return m_c2sHMacAlgo; }
    QByteArray hMacAlgoServerToClient() const { return m_s2cHMacAlgo; }

private:
    QByteArray hashAlgoForKexAlgo() const;
    void determineHashingAlgorithm(const SshKeyExchangeInit &kexInit, bool serverToClient);
    void checkHostKey(const QByteArray &hostKey);
    Q_NORETURN void throwHostKeyException();

    QByteArray m_serverId;
    QByteArray m_clientKexInitPayload;
    QByteArray m_serverKexInitPayload;
    QScopedPointer<Botan::DH_PrivateKey> m_dhKey;
    QScopedPointer<Botan::ECDH_PrivateKey> m_ecdhKey;
    QByteArray m_kexAlgoName;
    QByteArray m_k;
    QByteArray m_h;
    QByteArray m_serverHostKeyAlgo;
    QByteArray m_encryptionAlgo;
    QByteArray m_decryptionAlgo;
    QByteArray m_c2sHMacAlgo;
    QByteArray m_s2cHMacAlgo;
    std::unique_ptr<Botan::HashFunction> m_hash;
    const SshConnectionParameters m_connParams;
    SshSendFacility &m_sendFacility;
    QByteArray m_hostFingerprint;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHKEYEXCHANGE_P_H
