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

#include "sshcryptofacility_p.h"
#include "sshpacketparser_p.h"

#include <QStringList>

namespace QSsh {
namespace Internal {

class SshKeyExchange;

struct SshKeyExchangeInit
{
    char cookie[16];
    SshNameList keyAlgorithms;
    SshNameList serverHostKeyAlgorithms;
    SshNameList encryptionAlgorithmsClientToServer;
    SshNameList encryptionAlgorithmsServerToClient;
    SshNameList macAlgorithmsClientToServer;
    SshNameList macAlgorithmsServerToClient;
    SshNameList compressionAlgorithmsClientToServer;
    SshNameList compressionAlgorithmsServerToClient;
    SshNameList languagesClientToServer;
    SshNameList languagesServerToClient;
    bool firstKexPacketFollows;
};

struct SshKeyExchangeReply
{
    QByteArray k_s;
    QList<Botan::BigInt> hostKeyParameters; // DSS: p, q, g, y. RSA: e, n.
    QByteArray q; // For ECDSA host keys only.
    Botan::BigInt f; // For DH only.
    QByteArray q_s; // For ECDH only.
    QByteArray signatureBlob;
};

struct SshDisconnect
{
    quint32 reasonCode;
    QString description;
    QByteArray language;
};

struct SshUserAuthBanner
{
    QString message;
    QByteArray language;
};

struct SshUserAuthInfoRequestPacket
{
    QString name;
    QString instruction;
    QByteArray languageTag;
    QStringList prompts;
    QList<bool> echos;
};

struct SshDebug
{
    bool display;
    QString message;
    QByteArray language;
};

struct SshUnimplemented
{
    quint32 invalidMsgSeqNr;
};

struct SshRequestSuccess
{
    quint32 bindPort;
};

struct SshChannelOpen
{
    quint32 remoteChannel;
    quint32 remoteWindowSize;
    quint32 remoteMaxPacketSize;
    QByteArray remoteAddress;
    quint32 remotePort;
};

struct SshChannelOpenFailure
{
    quint32 localChannel;
    quint32 reasonCode;
    QString reasonString;
    QByteArray language;
};

struct SshChannelOpenConfirmation
{
    quint32 localChannel;
    quint32 remoteChannel;
    quint32 remoteWindowSize;
    quint32 remoteMaxPacketSize;
};

struct SshChannelWindowAdjust
{
    quint32 localChannel;
    quint32 bytesToAdd;
};

struct SshChannelData
{
    quint32 localChannel;
    QByteArray data;
};

struct SshChannelExtendedData
{
    quint32 localChannel;
    quint32 type;
    QByteArray data;
};

struct SshChannelExitStatus
{
    quint32 localChannel;
    quint32 exitStatus;
};

struct SshChannelExitSignal
{
    quint32 localChannel;
    QByteArray signal;
    bool coreDumped;
    QString error;
    QByteArray language;
};

class SshIncomingPacket : public AbstractSshPacket
{
public:
    SshIncomingPacket();

    void consumeData(QByteArray &data);
    void recreateKeys(const SshKeyExchange &keyExchange);
    void reset();

    SshKeyExchangeInit extractKeyExchangeInitData() const;
    SshKeyExchangeReply extractKeyExchangeReply(const QByteArray &kexAlgo,
                                                const QByteArray &hostKeyAlgo) const;
    SshDisconnect extractDisconnect() const;
    SshUserAuthBanner extractUserAuthBanner() const;
    SshUserAuthInfoRequestPacket extractUserAuthInfoRequest() const;
    SshDebug extractDebug() const;
    SshRequestSuccess extractRequestSuccess() const;
    SshUnimplemented extractUnimplemented() const;

    SshChannelOpen extractChannelOpen() const;
    SshChannelOpenFailure extractChannelOpenFailure() const;
    SshChannelOpenConfirmation extractChannelOpenConfirmation() const;
    SshChannelWindowAdjust extractWindowAdjust() const;
    SshChannelData extractChannelData() const;
    SshChannelExtendedData extractChannelExtendedData() const;
    SshChannelExitStatus extractChannelExitStatus() const;
    SshChannelExitSignal extractChannelExitSignal() const;
    quint32 extractRecipientChannel() const;
    QByteArray extractChannelRequestType() const;

    quint32 serverSeqNr() const { return m_serverSeqNr; }

    static const QByteArray ExitStatusType;
    static const QByteArray ExitSignalType;
    static const QByteArray ForwardedTcpIpType;

private:
    virtual quint32 cipherBlockSize() const;
    virtual quint32 macLength() const;
    virtual void calculateLength() const;

    void decrypt();
    void moveFirstBytes(QByteArray &target, QByteArray &source, int n);

    quint32 m_serverSeqNr;
    SshDecryptionFacility m_decrypter;
};

} // namespace Internal
} // namespace QSsh
