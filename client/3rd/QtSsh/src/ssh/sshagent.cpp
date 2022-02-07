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
#include "sshagent_p.h"

#include "sshlogging_p.h"
#include "sshpacket_p.h"
#include "sshpacketparser_p.h"
#include "ssh_global.h"

#include <QTimer>
#include <QtEndian>

#include <algorithm>

namespace QSsh {
namespace Internal {

// https://github.com/openssh/openssh-portable/blob/V_7_2/PROTOCOL.agent
enum PacketType {
    SSH_AGENT_FAILURE = 5,
    SSH2_AGENTC_REQUEST_IDENTITIES = 11,
    SSH2_AGENTC_SIGN_REQUEST = 13,
    SSH2_AGENT_IDENTITIES_ANSWER = 12,
    SSH2_AGENT_SIGN_RESPONSE = 14,
};

// TODO: Remove once we require 5.7, where the endianness functions have a sane input type.
template<typename T> static T fromBigEndian(const QByteArray &ba)
{
    return qFromBigEndian<T>(reinterpret_cast<const uchar *>(ba.constData()));
}

void SshAgent::refreshKeysImpl()
{
    if (state() != Connected)
        return;
    const auto keysRequestIt = std::find_if(m_pendingRequests.constBegin(),
            m_pendingRequests.constEnd(), [](const Request &r) { return r.isKeysRequest(); });
    if (keysRequestIt != m_pendingRequests.constEnd()) {
        qCDebug(sshLog) << "keys request already pending, not adding another one";
        return;
    }
    qCDebug(sshLog) << "queueing keys request";
    m_pendingRequests << Request();
    sendNextRequest();
}

void SshAgent::requestSignatureImpl(const QByteArray &key, uint token)
{
    if (state() != Connected)
        return;
    const QByteArray data = m_dataToSign.take(qMakePair(key, token));
    QSSH_ASSERT(!data.isEmpty());
    qCDebug(sshLog) << "queueing signature request";
    m_pendingRequests.enqueue(Request(key, data, token));
    sendNextRequest();
}

void SshAgent::sendNextRequest()
{
    if (m_pendingRequests.isEmpty())
        return;
    if (m_outgoingPacket.isComplete())
        return;
    if (hasError())
        return;
    const Request &request = m_pendingRequests.head();
    m_outgoingPacket = request.isKeysRequest() ? generateKeysPacket() : generateSigPacket(request);
    sendPacket();
}

SshAgent::Packet SshAgent::generateKeysPacket()
{
    qCDebug(sshLog) << "requesting keys from agent";
    Packet p;
    p.size = 1;
    p.data += char(SSH2_AGENTC_REQUEST_IDENTITIES);
    return p;
}

SshAgent::Packet SshAgent::generateSigPacket(const SshAgent::Request &request)
{
    qCDebug(sshLog) << "requesting signature from agent for key" << request.key << "and token"
                    << request.token;
    Packet p;
    p.data += char(SSH2_AGENTC_SIGN_REQUEST);
    p.data += AbstractSshPacket::encodeString(request.key);
    p.data += AbstractSshPacket::encodeString(request.dataToSign);
    p.data += AbstractSshPacket::encodeInt(quint32(0));
    p.size = p.data.count();
    return p;
}

SshAgent::~SshAgent()
{
    m_agentSocket.disconnect(this);
}

void SshAgent::storeDataToSign(const QByteArray &key, const QByteArray &data, uint token)
{
    instance().m_dataToSign.insert(qMakePair(key, token), data);
}

void SshAgent::removeDataToSign(const QByteArray &key, uint token)
{
    instance().m_dataToSign.remove(qMakePair(key, token));
}

SshAgent &QSsh::Internal::SshAgent::instance()
{
    static SshAgent agent;
    return agent;
}

SshAgent::SshAgent()
{
    connect(&m_agentSocket, &QLocalSocket::connected, this, &SshAgent::handleConnected);
    connect(&m_agentSocket, &QLocalSocket::disconnected, this, &SshAgent::handleDisconnected);
    connect(&m_agentSocket, SIGNAL(error(QAbstractSocket::SocketError)), this,
        SLOT(handleSocketError()));
    connect(&m_agentSocket, &QLocalSocket::readyRead, this, &SshAgent::handleIncomingData);
    QTimer::singleShot(0, this, &SshAgent::connectToServer);
}

void SshAgent::connectToServer()
{
    const QByteArray serverAddress = qgetenv("SSH_AUTH_SOCK");
    if (serverAddress.isEmpty()) {
        qCDebug(sshLog) << "agent failure: socket address unknown";
        m_error = tr("Cannot connect to ssh-agent: SSH_AUTH_SOCK is not set.");
        emit errorOccurred();
        return;
    }
    qCDebug(sshLog) << "connecting to ssh-agent socket" << serverAddress;
    m_state = Connecting;
    m_agentSocket.connectToServer(QString::fromLocal8Bit(serverAddress));
}

void SshAgent::handleConnected()
{
    m_state = Connected;
    qCDebug(sshLog) << "connection to ssh-agent established";
    refreshKeys();
}

void SshAgent::handleDisconnected()
{
    qCDebug(sshLog) << "lost connection to ssh-agent";
    m_error = tr("Lost connection to ssh-agent for unknown reason.");
    setDisconnected();
}

void SshAgent::handleSocketError()
{
    qCDebug(sshLog) << "agent socket error" << m_agentSocket.error();
    m_error = m_agentSocket.errorString();
    setDisconnected();
}

void SshAgent::handleIncomingData()
{
    qCDebug(sshLog) << "getting data from agent";
    m_incomingData += m_agentSocket.readAll();
    while (!hasError() && !m_incomingData.isEmpty()) {
        if (m_incomingPacket.size == 0) {
            if (m_incomingData.count() < int(sizeof m_incomingPacket.size))
                break;
            m_incomingPacket.size = fromBigEndian<quint32>(m_incomingData);
            m_incomingData.remove(0, sizeof m_incomingPacket.size);
        }
        const int bytesToTake = qMin<quint32>(m_incomingPacket.size - m_incomingPacket.data.count(),
                                              m_incomingData.count());
        m_incomingPacket.data += m_incomingData.left(bytesToTake);
        m_incomingData.remove(0, bytesToTake);
        if (m_incomingPacket.isComplete())
            handleIncomingPacket();
        else
            break;
    }
}

void SshAgent::handleIncomingPacket()
{
    try {
        qCDebug(sshLog) << "received packet from agent:" << m_incomingPacket.data.toHex();
        const char messageType = m_incomingPacket.data.at(0);
        switch (messageType) {
        case SSH2_AGENT_IDENTITIES_ANSWER:
            handleIdentitiesPacket();
            break;
        case SSH2_AGENT_SIGN_RESPONSE:
            handleSignaturePacket();
            break;
        case SSH_AGENT_FAILURE:
            if (m_pendingRequests.isEmpty()) {
                qCWarning(sshLog) << "unexpected failure message from agent";
            } else {
                const Request request = m_pendingRequests.dequeue();
                if (request.isSignatureRequest()) {
                    qCWarning(sshLog) << "agent failed to sign message for key"
                                      << request.key.toHex();
                    emit signatureAvailable(request.key, QByteArray(), request.token);
                } else {
                    qCWarning(sshLog) << "agent failed to retrieve key list";
                    if (m_keys.isEmpty()) {
                        m_error = tr("ssh-agent failed to retrieve keys.");
                        setDisconnected();
                    }
                }
            }
            break;
        default:
            qCWarning(sshLog) << "unexpected message type from agent:" << messageType;
        }
    } catch (const SshPacketParseException &) {
        qCWarning(sshLog()) << "received malformed packet from agent";
        handleProtocolError();
    }
    m_incomingPacket.invalidate();
    m_incomingPacket.size = 0;
    m_outgoingPacket.invalidate();
    sendNextRequest();
}

void SshAgent::handleIdentitiesPacket()
{
    qCDebug(sshLog) << "got keys packet from agent";
    if (m_pendingRequests.isEmpty() || !m_pendingRequests.dequeue().isKeysRequest()) {
        qCDebug(sshLog) << "packet was not requested";
        handleProtocolError();
        return;
    }
    quint32 offset = 1;
    const auto keyCount = SshPacketParser::asUint32(m_incomingPacket.data, &offset);
    qCDebug(sshLog) << "packet contains" << keyCount << "keys";
    QList<QByteArray> newKeys;
    for (quint32 i = 0; i < keyCount; ++i) {
        const QByteArray key = SshPacketParser::asString(m_incomingPacket.data, &offset);
        quint32 keyOffset = 0;
        const QByteArray algoName = SshPacketParser::asString(key, &keyOffset);
        SshPacketParser::asString(key, &keyOffset); // rest of key blob
        SshPacketParser::asString(m_incomingPacket.data, &offset); // comment
        qCDebug(sshLog) << "adding key of type" << algoName;
        newKeys << key;
    }

    m_keys = newKeys;
    emit keysUpdated();
}

void SshAgent::handleSignaturePacket()
{
    qCDebug(sshLog) << "got signature packet from agent";
    if (m_pendingRequests.isEmpty()) {
        qCDebug(sshLog) << "signature packet was not requested";
        handleProtocolError();
        return;
    }
    const Request request = m_pendingRequests.dequeue();
    if (!request.isSignatureRequest()) {
        qCDebug(sshLog) << "signature packet was not requested";
        handleProtocolError();
        return;
    }
    const QByteArray signature = SshPacketParser::asString(m_incomingPacket.data, 1);
    qCDebug(sshLog) << "signature for key" << request.key.toHex() << "is" << signature.toHex();
    emit signatureAvailable(request.key, signature, request.token);
}

void SshAgent::handleProtocolError()
{
    m_error = tr("Protocol error when talking to ssh-agent.");
    setDisconnected();
}

void SshAgent::setDisconnected()
{
    m_state = Unconnected;
    m_agentSocket.disconnect(this);
    emit errorOccurred();
}

void SshAgent::sendPacket()
{
    const quint32 sizeMsb = qToBigEndian(m_outgoingPacket.size);
    m_agentSocket.write(reinterpret_cast<const char *>(&sizeMsb), sizeof sizeMsb);
    m_agentSocket.write(m_outgoingPacket.data);
}

} // namespace Internal
} // namespace QSsh
