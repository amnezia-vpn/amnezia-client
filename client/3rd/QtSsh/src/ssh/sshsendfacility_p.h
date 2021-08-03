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

#ifndef SSHCONNECTIONOUTSTATE_P_H
#define SSHCONNECTIONOUTSTATE_P_H

#include "sshcryptofacility_p.h"
#include "sshoutgoingpacket_p.h"

#include <QStringList>

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE


namespace QSsh {
class SshPseudoTerminal;

namespace Internal {
class SshKeyExchange;

class SshSendFacility
{
public:
    SshSendFacility(QTcpSocket *socket);
    void reset();
    void recreateKeys(const SshKeyExchange &keyExchange);
    void createAuthenticationKey(const QByteArray &privKeyFileContents);

    QByteArray sessionId() const { return m_encrypter.sessionId(); }

    QByteArray sendKeyExchangeInitPacket();
    void sendKeyDhInitPacket(const Botan::BigInt &e);
    void sendKeyEcdhInitPacket(const QByteArray &clientQ);
    void sendNewKeysPacket();
    void sendDisconnectPacket(SshErrorCode reason,
        const QByteArray &reasonString);
    void sendMsgUnimplementedPacket(quint32 serverSeqNr);
    void sendUserAuthServiceRequestPacket();
    void sendUserAuthByPasswordRequestPacket(const QByteArray &user,
        const QByteArray &service, const QByteArray &pwd);
    void sendUserAuthByPublicKeyRequestPacket(const QByteArray &user,
        const QByteArray &service, const QByteArray &key, const QByteArray &signature);
    void sendQueryPublicKeyPacket(const QByteArray &user, const QByteArray &service,
                                  const QByteArray &publicKey);
    void sendUserAuthByKeyboardInteractiveRequestPacket(const QByteArray &user,
        const QByteArray &service);
    void sendUserAuthInfoResponsePacket(const QStringList &responses);
    void sendRequestFailurePacket();
    void sendIgnorePacket();
    void sendInvalidPacket();
    void sendSessionPacket(quint32 channelId, quint32 windowSize,
        quint32 maxPacketSize);
    void sendDirectTcpIpPacket(quint32 channelId, quint32 windowSize, quint32 maxPacketSize,
        const QByteArray &remoteHost, quint32 remotePort, const QByteArray &localIpAddress,
        quint32 localPort);
    void sendTcpIpForwardPacket(const QByteArray &bindAddress, quint32 bindPort);
    void sendCancelTcpIpForwardPacket(const QByteArray &bindAddress, quint32 bindPort);
    void sendPtyRequestPacket(quint32 remoteChannel,
        const SshPseudoTerminal &terminal);
    void sendEnvPacket(quint32 remoteChannel, const QByteArray &var,
        const QByteArray &value);
    void sendX11ForwardingPacket(quint32 remoteChannel, const QByteArray &protocol,
                                 const QByteArray &cookie, quint32 screenNumber);
    void sendExecPacket(quint32 remoteChannel, const QByteArray &command);
    void sendShellPacket(quint32 remoteChannel);
    void sendSftpPacket(quint32 remoteChannel);
    void sendWindowAdjustPacket(quint32 remoteChannel, quint32 bytesToAdd);
    void sendChannelDataPacket(quint32 remoteChannel, const QByteArray &data);
    void sendChannelSignalPacket(quint32 remoteChannel,
        const QByteArray &signalName);
    void sendChannelEofPacket(quint32 remoteChannel);
    void sendChannelClosePacket(quint32 remoteChannel);
    void sendChannelOpenConfirmationPacket(quint32 remoteChannel, quint32 localChannel,
        quint32 localWindowSize, quint32 maxPackeSize);
    void sendChannelOpenFailurePacket(quint32 remoteChannel, quint32 reason,
        const QByteArray &reasonString);
    quint32 nextClientSeqNr() const { return m_clientSeqNr; }

    bool encrypterIsValid() const { return m_encrypter.isValid(); }

private:
    void sendPacket();

    quint32 m_clientSeqNr;
    SshEncryptionFacility m_encrypter;
    QTcpSocket *m_socket;
    SshOutgoingPacket m_outgoingPacket;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHCONNECTIONOUTSTATE_P_H
