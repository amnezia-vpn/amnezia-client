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

#include "sshoutgoingpacket_p.h"

#include "sshcapabilities_p.h"
#include "sshcryptofacility_p.h"
#include "sshlogging_p.h"

#include <QtEndian>

namespace QSsh {
namespace Internal {

SshOutgoingPacket::SshOutgoingPacket(const SshEncryptionFacility &encrypter,
    const quint32 &seqNr) : m_encrypter(encrypter), m_seqNr(seqNr)
{
}

quint32 SshOutgoingPacket::cipherBlockSize() const
{
    return qMax(m_encrypter.cipherBlockSize(), 4U);
}

quint32 SshOutgoingPacket::macLength() const
{
    return m_encrypter.macLength();
}

QByteArray SshOutgoingPacket::generateKeyExchangeInitPacket()
{
    const QByteArray &supportedkeyExchangeMethods
        = encodeNameList(SshCapabilities::KeyExchangeMethods);
    const QByteArray &supportedPublicKeyAlgorithms
        = encodeNameList(SshCapabilities::PublicKeyAlgorithms);
    const QByteArray &supportedEncryptionAlgorithms
        = encodeNameList(SshCapabilities::EncryptionAlgorithms);
    const QByteArray &supportedMacAlgorithms
        = encodeNameList(SshCapabilities::MacAlgorithms);
    const QByteArray &supportedCompressionAlgorithms
        = encodeNameList(SshCapabilities::CompressionAlgorithms);
    const QByteArray &supportedLanguages = encodeNameList(QList<QByteArray>());

    init(SSH_MSG_KEXINIT);
    m_data += m_encrypter.getRandomNumbers(16);
    m_data.append(supportedkeyExchangeMethods);
    m_data.append(supportedPublicKeyAlgorithms);
    m_data.append(supportedEncryptionAlgorithms)
        .append(supportedEncryptionAlgorithms);
    m_data.append(supportedMacAlgorithms).append(supportedMacAlgorithms);
    m_data.append(supportedCompressionAlgorithms)
        .append(supportedCompressionAlgorithms);
    m_data.append(supportedLanguages).append(supportedLanguages);
    appendBool(false); // No guessed packet.
    m_data.append(QByteArray(4, 0)); // Reserved.
    QByteArray payload = m_data.mid(PayloadOffset);
    finalize();
    return payload;
}

void SshOutgoingPacket::generateKeyDhInitPacket(const Botan::BigInt &e)
{
    init(SSH_MSG_KEXDH_INIT).appendMpInt(e).finalize();
}

void SshOutgoingPacket::generateKeyEcdhInitPacket(const QByteArray &clientQ)
{
    init(SSH_MSG_KEX_ECDH_INIT).appendString(clientQ).finalize();
}

void SshOutgoingPacket::generateNewKeysPacket()
{
    init(SSH_MSG_NEWKEYS).finalize();
}

void SshOutgoingPacket::generateUserAuthServiceRequestPacket()
{
    generateServiceRequest("ssh-userauth");
}

void SshOutgoingPacket::generateServiceRequest(const QByteArray &service)
{
    init(SSH_MSG_SERVICE_REQUEST).appendString(service).finalize();
}

void SshOutgoingPacket::generateUserAuthByPasswordRequestPacket(const QByteArray &user,
    const QByteArray &service, const QByteArray &pwd)
{
    init(SSH_MSG_USERAUTH_REQUEST).appendString(user).appendString(service);
    if (pwd.isEmpty())
        appendString("none"); // RFC 4252, 5.2
    else
        appendString("password").appendBool(false).appendString(pwd);
    finalize();
}

void SshOutgoingPacket::generateUserAuthByPublicKeyRequestPacket(const QByteArray &user,
    const QByteArray &service)
{
    init(SSH_MSG_USERAUTH_REQUEST).appendString(user).appendString(service)
        .appendString("publickey").appendBool(true)
        .appendString(m_encrypter.authenticationAlgorithmName())
        .appendString(m_encrypter.authenticationPublicKey());
    const QByteArray &dataToSign = m_data.mid(PayloadOffset);
    appendString(m_encrypter.authenticationKeySignature(dataToSign));
    finalize();
}

void SshOutgoingPacket::generateUserAuthByKeyboardInteractiveRequestPacket(const QByteArray &user,
                                                                           const QByteArray &service)
{
    // RFC 4256, 3.1
    init(SSH_MSG_USERAUTH_REQUEST).appendString(user).appendString(service)
            .appendString("keyboard-interactive")
            .appendString(QByteArray()) // Language tag. Deprecated and should be empty
            .appendString(QByteArray()) // Submethods.
            .finalize();
}

void SshOutgoingPacket::generateUserAuthInfoResponsePacket(const QStringList &responses)
{
    // RFC 4256, 3.4
    init(SSH_MSG_USERAUTH_INFO_RESPONSE).appendInt(responses.count());
    foreach (const QString &response, responses)
        appendString(response.toUtf8());
    finalize();
}

void SshOutgoingPacket::generateRequestFailurePacket()
{
    init(SSH_MSG_REQUEST_FAILURE).finalize();
}

void SshOutgoingPacket::generateIgnorePacket()
{
    init(SSH_MSG_IGNORE).finalize();
}

void SshOutgoingPacket::generateInvalidMessagePacket()
{
    init(SSH_MSG_INVALID).finalize();
}

void SshOutgoingPacket::generateSessionPacket(quint32 channelId,
    quint32 windowSize, quint32 maxPacketSize)
{
    init(SSH_MSG_CHANNEL_OPEN).appendString("session").appendInt(channelId)
            .appendInt(windowSize).appendInt(maxPacketSize).finalize();
}

void SshOutgoingPacket::generateDirectTcpIpPacket(quint32 channelId, quint32 windowSize,
        quint32 maxPacketSize, const QByteArray &remoteHost, quint32 remotePort,
        const QByteArray &localIpAddress, quint32 localPort)
{
    init(SSH_MSG_CHANNEL_OPEN).appendString("direct-tcpip").appendInt(channelId)
            .appendInt(windowSize).appendInt(maxPacketSize).appendString(remoteHost)
            .appendInt(remotePort).appendString(localIpAddress).appendInt(localPort).finalize();
}

void SshOutgoingPacket::generateTcpIpForwardPacket(const QByteArray &bindAddress, quint32 bindPort)
{
    init(SSH_MSG_GLOBAL_REQUEST).appendString("tcpip-forward").appendBool(true)
            .appendString(bindAddress).appendInt(bindPort).finalize();
}

void SshOutgoingPacket::generateCancelTcpIpForwardPacket(const QByteArray &bindAddress,
                                                         quint32 bindPort)
{
    init(SSH_MSG_GLOBAL_REQUEST).appendString("cancel-tcpip-forward").appendBool(true)
            .appendString(bindAddress).appendInt(bindPort).finalize();
}

void SshOutgoingPacket::generateEnvPacket(quint32 remoteChannel,
    const QByteArray &var, const QByteArray &value)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel).appendString("env")
        .appendBool(false).appendString(var).appendString(value).finalize();
}

void SshOutgoingPacket::generatePtyRequestPacket(quint32 remoteChannel,
    const SshPseudoTerminal &terminal)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel)
        .appendString("pty-req").appendBool(false)
        .appendString(terminal.termType).appendInt(terminal.columnCount)
        .appendInt(terminal.rowCount).appendInt(0).appendInt(0);
    QByteArray modeString;
    for (SshPseudoTerminal::ModeMap::ConstIterator it = terminal.modes.constBegin();
         it != terminal.modes.constEnd(); ++it) {
        modeString += char(it.key());
        modeString += encodeInt(it.value());
    }
    modeString += char(0); // TTY_OP_END
    appendString(modeString).finalize();
}

void SshOutgoingPacket::generateExecPacket(quint32 remoteChannel,
    const QByteArray &command)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel).appendString("exec")
        .appendBool(true).appendString(command).finalize();
}

void SshOutgoingPacket::generateShellPacket(quint32 remoteChannel)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel).appendString("shell")
        .appendBool(true).finalize();
}

void SshOutgoingPacket::generateSftpPacket(quint32 remoteChannel)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel)
        .appendString("subsystem").appendBool(true).appendString("sftp")
        .finalize();
}

void SshOutgoingPacket::generateWindowAdjustPacket(quint32 remoteChannel,
    quint32 bytesToAdd)
{
    init(SSH_MSG_CHANNEL_WINDOW_ADJUST).appendInt(remoteChannel)
        .appendInt(bytesToAdd).finalize();
}

void SshOutgoingPacket::generateChannelDataPacket(quint32 remoteChannel,
    const QByteArray &data)
{
    init(SSH_MSG_CHANNEL_DATA).appendInt(remoteChannel).appendString(data)
        .finalize();
}

void SshOutgoingPacket::generateChannelSignalPacket(quint32 remoteChannel,
    const QByteArray &signalName)
{
    init(SSH_MSG_CHANNEL_REQUEST).appendInt(remoteChannel)
        .appendString("signal").appendBool(false).appendString(signalName)
        .finalize();
}

void SshOutgoingPacket::generateChannelEofPacket(quint32 remoteChannel)
{
    init(SSH_MSG_CHANNEL_EOF).appendInt(remoteChannel).finalize();
}

void SshOutgoingPacket::generateChannelClosePacket(quint32 remoteChannel)
{
    init(SSH_MSG_CHANNEL_CLOSE).appendInt(remoteChannel).finalize();
}

void SshOutgoingPacket::generateChannelOpenConfirmationPacket(quint32 remoteChannel,
                                                              quint32 localChannel,
                                                              quint32 localWindowSize,
                                                              quint32 maxPacketSize)
{
    init(SSH_MSG_CHANNEL_OPEN_CONFIRMATION).appendInt(remoteChannel).appendInt(localChannel)
            .appendInt(localWindowSize).appendInt(maxPacketSize).finalize();
}

void SshOutgoingPacket::generateChannelOpenFailurePacket(quint32 remoteChannel, quint32 reason,
                                                         const QByteArray &reasonString)
{
    init(SSH_MSG_CHANNEL_OPEN_FAILURE).appendInt(remoteChannel).appendInt(reason)
            .appendString(reasonString).appendString(QByteArray()).finalize();
}

void SshOutgoingPacket::generateDisconnectPacket(SshErrorCode reason,
    const QByteArray &reasonString)
{
    init(SSH_MSG_DISCONNECT).appendInt(reason).appendString(reasonString)
        .appendString(QByteArray()).finalize();
}

void SshOutgoingPacket::generateMsgUnimplementedPacket(quint32 serverSeqNr)
{
    init(SSH_MSG_UNIMPLEMENTED).appendInt(serverSeqNr).finalize();
}

SshOutgoingPacket &SshOutgoingPacket::appendInt(quint32 val)
{
    m_data.append(encodeInt(val));
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::appendMpInt(const Botan::BigInt &number)
{
    m_data.append(encodeMpInt(number));
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::appendBool(bool b)
{
    m_data += static_cast<char>(b);
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::appendString(const QByteArray &string)
{
    m_data.append(encodeString(string));
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::init(SshPacketType type)
{
    m_data.resize(TypeOffset + 1);
    m_data[TypeOffset] = type;
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::setPadding()
{
    m_data += m_encrypter.getRandomNumbers(MinPaddingLength);
    int padLength = MinPaddingLength;
    const int divisor = sizeDivisor();
    const int mod = m_data.size() % divisor;
    padLength += divisor - mod;
    m_data += m_encrypter.getRandomNumbers(padLength - MinPaddingLength);
    m_data[PaddingLengthOffset] = padLength;
    return *this;
}

SshOutgoingPacket &SshOutgoingPacket::encrypt()
{
    const QByteArray &mac
        = generateMac(m_encrypter, m_seqNr);
    m_encrypter.encrypt(m_data);
    m_data += mac;
    return *this;
}

void SshOutgoingPacket::finalize()
{
    setPadding();
    setLengthField(m_data);
    m_length = m_data.size() - 4;
    qCDebug(sshLog, "Encrypting packet of type %u", m_data.at(TypeOffset));
    encrypt();
    qCDebug(sshLog, "Sending packet of size %d", rawData().count());
    Q_ASSERT(isComplete());
}

int SshOutgoingPacket::sizeDivisor() const
{
    return qMax(cipherBlockSize(), 8U);
}

QByteArray SshOutgoingPacket::encodeNameList(const QList<QByteArray> &list)
{
    QByteArray data;
    data.resize(4);
    for (int i = 0; i < list.count(); ++i) {
        if (i > 0)
            data.append(',');
        data.append(list.at(i));
    }
    AbstractSshPacket::setLengthField(data);
    return data;
}

} // namespace Internal
} // namespace QSsh
