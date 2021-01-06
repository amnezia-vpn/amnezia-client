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
        const QByteArray &service);
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

private:
    void sendPacket();

    quint32 m_clientSeqNr;
    SshEncryptionFacility m_encrypter;
    QTcpSocket *m_socket;
    SshOutgoingPacket m_outgoingPacket;
};

} // namespace Internal
} // namespace QSsh
