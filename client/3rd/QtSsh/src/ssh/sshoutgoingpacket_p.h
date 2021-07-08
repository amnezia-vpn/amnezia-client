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

#include "sshpacket_p.h"

#include "sshpseudoterminal.h"

#include <QStringList>

namespace QSsh {
namespace Internal {

class SshEncryptionFacility;

class SshOutgoingPacket : public AbstractSshPacket
{
public:
    SshOutgoingPacket(const SshEncryptionFacility &encrypter,
        const quint32 &seqNr);

    QByteArray generateKeyExchangeInitPacket(); // Returns payload.
    void generateKeyDhInitPacket(const Botan::BigInt &e);
    void generateKeyEcdhInitPacket(const QByteArray &clientQ);
    void generateNewKeysPacket();
    void generateDisconnectPacket(SshErrorCode reason,
        const QByteArray &reasonString);
    void generateMsgUnimplementedPacket(quint32 serverSeqNr);
    void generateUserAuthServiceRequestPacket();
    void generateUserAuthByPasswordRequestPacket(const QByteArray &user,
        const QByteArray &service, const QByteArray &pwd);
    void generateUserAuthByPublicKeyRequestPacket(const QByteArray &user,
        const QByteArray &service);
    void generateUserAuthByKeyboardInteractiveRequestPacket(const QByteArray &user,
        const QByteArray &service);
    void generateUserAuthInfoResponsePacket(const QStringList &responses);
    void generateRequestFailurePacket();
    void generateIgnorePacket();
    void generateInvalidMessagePacket();
    void generateSessionPacket(quint32 channelId, quint32 windowSize,
        quint32 maxPacketSize);
    void generateDirectTcpIpPacket(quint32 channelId, quint32 windowSize,
        quint32 maxPacketSize, const QByteArray &remoteHost, quint32 remotePort,
        const QByteArray &localIpAddress, quint32 localPort);
    void generateTcpIpForwardPacket(const QByteArray &bindAddress, quint32 bindPort);
    void generateCancelTcpIpForwardPacket(const QByteArray &bindAddress, quint32 bindPort);
    void generateEnvPacket(quint32 remoteChannel, const QByteArray &var,
        const QByteArray &value);
    void generatePtyRequestPacket(quint32 remoteChannel,
        const SshPseudoTerminal &terminal);
    void generateExecPacket(quint32 remoteChannel, const QByteArray &command);
    void generateShellPacket(quint32 remoteChannel);
    void generateSftpPacket(quint32 remoteChannel);
    void generateWindowAdjustPacket(quint32 remoteChannel, quint32 bytesToAdd);
    void generateChannelDataPacket(quint32 remoteChannel,
        const QByteArray &data);
    void generateChannelSignalPacket(quint32 remoteChannel,
        const QByteArray &signalName);
    void generateChannelEofPacket(quint32 remoteChannel);
    void generateChannelClosePacket(quint32 remoteChannel);
    void generateChannelOpenConfirmationPacket(quint32 remoteChannel, quint32 localChannel,
        quint32 localWindowSize, quint32 maxPackeSize);
    void generateChannelOpenFailurePacket(quint32 remoteChannel, quint32 reason,
        const QByteArray &reasonString);

private:
    virtual quint32 cipherBlockSize() const;
    virtual quint32 macLength() const;

    static QByteArray encodeNameList(const QList<QByteArray> &list);

    void generateServiceRequest(const QByteArray &service);

    SshOutgoingPacket &init(SshPacketType type);
    SshOutgoingPacket &setPadding();
    SshOutgoingPacket &encrypt();
    void finalize();

    SshOutgoingPacket &appendInt(quint32 val);
    SshOutgoingPacket &appendString(const QByteArray &string);
    SshOutgoingPacket &appendMpInt(const Botan::BigInt &number);
    SshOutgoingPacket &appendBool(bool b);
    int sizeDivisor() const;

    const SshEncryptionFacility &m_encrypter;
    const quint32 &m_seqNr;
};

} // namespace Internal
} // namespace QSsh
