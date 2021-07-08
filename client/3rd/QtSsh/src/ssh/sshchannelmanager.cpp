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

#include "sshchannelmanager_p.h"

#include "sftpchannel.h"
#include "sftpchannel_p.h"
#include "sshdirecttcpiptunnel.h"
#include "sshdirecttcpiptunnel_p.h"
#include "sshforwardedtcpiptunnel.h"
#include "sshforwardedtcpiptunnel_p.h"
#include "sshincomingpacket_p.h"
#include "sshlogging_p.h"
#include "sshremoteprocess.h"
#include "sshremoteprocess_p.h"
#include "sshsendfacility_p.h"
#include "sshtcpipforwardserver.h"
#include "sshtcpipforwardserver_p.h"

#include <QList>

namespace QSsh {
namespace Internal {

SshChannelManager::SshChannelManager(SshSendFacility &sendFacility,
    QObject *parent)
    : QObject(parent), m_sendFacility(sendFacility), m_nextLocalChannelId(0)
{
}

void SshChannelManager::handleChannelRequest(const SshIncomingPacket &packet)
{
    lookupChannel(packet.extractRecipientChannel())
        ->handleChannelRequest(packet);
}

void SshChannelManager::handleChannelOpen(const SshIncomingPacket &packet)
{
    SshChannelOpen channelOpen = packet.extractChannelOpen();

    SshTcpIpForwardServer::Ptr server;

    foreach (const SshTcpIpForwardServer::Ptr &candidate, m_listeningForwardServers) {
        if (candidate->port() == channelOpen.remotePort
                && candidate->bindAddress().toUtf8() == channelOpen.remoteAddress) {
            server = candidate;
            break;
        }
    };


    if (server.isNull()) {
        // Apparently the server knows a remoteAddress we are not aware of. There are plenty of ways
        // to make that happen: /etc/hosts on the server, different writings for localhost,
        // different DNS servers, ...
        // Rather than trying to figure that out, we just use the first listening forwarder with the
        // same port.
        foreach (const SshTcpIpForwardServer::Ptr &candidate, m_listeningForwardServers) {
            if (candidate->port() == channelOpen.remotePort) {
                server = candidate;
                break;
            }
        };
    }

    if (server.isNull()) {
        SshOpenFailureType reason = (channelOpen.remotePort == 0) ?
                    SSH_OPEN_UNKNOWN_CHANNEL_TYPE : SSH_OPEN_ADMINISTRATIVELY_PROHIBITED;
        try {
            m_sendFacility.sendChannelOpenFailurePacket(channelOpen.remoteChannel, reason,
                                                        QByteArray());
        }  catch (const std::exception &e) {
            qCWarning(sshLog, "Botan error: %s", e.what());
        }
        return;
    }

    SshForwardedTcpIpTunnel::Ptr tunnel(new SshForwardedTcpIpTunnel(m_nextLocalChannelId++,
                                                                    m_sendFacility));
    tunnel->d->handleOpenSuccess(channelOpen.remoteChannel, channelOpen.remoteWindowSize,
                                 channelOpen.remoteMaxPacketSize);
    tunnel->open(QIODevice::ReadWrite);
    server->setNewConnection(tunnel);
    insertChannel(tunnel->d, tunnel);
}

void SshChannelManager::handleChannelOpenFailure(const SshIncomingPacket &packet)
{
   const SshChannelOpenFailure &failure = packet.extractChannelOpenFailure();
   ChannelIterator it = lookupChannelAsIterator(failure.localChannel);
   try {
       it.value()->handleOpenFailure(failure.reasonString);
   } catch (const SshServerException &e) {
       removeChannel(it);
       throw e;
   }
   removeChannel(it);
}

void SshChannelManager::handleChannelOpenConfirmation(const SshIncomingPacket &packet)
{
   const SshChannelOpenConfirmation &confirmation
       = packet.extractChannelOpenConfirmation();
   lookupChannel(confirmation.localChannel)->handleOpenSuccess(confirmation.remoteChannel,
       confirmation.remoteWindowSize, confirmation.remoteMaxPacketSize);
}

void SshChannelManager::handleChannelSuccess(const SshIncomingPacket &packet)
{
    lookupChannel(packet.extractRecipientChannel())->handleChannelSuccess();
}

void SshChannelManager::handleChannelFailure(const SshIncomingPacket &packet)
{
    lookupChannel(packet.extractRecipientChannel())->handleChannelFailure();
}

void SshChannelManager::handleChannelWindowAdjust(const SshIncomingPacket &packet)
{
    const SshChannelWindowAdjust adjust = packet.extractWindowAdjust();
    lookupChannel(adjust.localChannel)->handleWindowAdjust(adjust.bytesToAdd);
}

void SshChannelManager::handleChannelData(const SshIncomingPacket &packet)
{
    const SshChannelData &data = packet.extractChannelData();
    lookupChannel(data.localChannel)->handleChannelData(data.data);
}

void SshChannelManager::handleChannelExtendedData(const SshIncomingPacket &packet)
{
    const SshChannelExtendedData &data = packet.extractChannelExtendedData();
    lookupChannel(data.localChannel)->handleChannelExtendedData(data.type, data.data);
}

void SshChannelManager::handleChannelEof(const SshIncomingPacket &packet)
{
    AbstractSshChannel * const channel
        = lookupChannel(packet.extractRecipientChannel(), true);
    if (channel)
        channel->handleChannelEof();
}

void SshChannelManager::handleChannelClose(const SshIncomingPacket &packet)
{
    const quint32 channelId = packet.extractRecipientChannel();

    ChannelIterator it = lookupChannelAsIterator(channelId, true);
    if (it != m_channels.end()) {
        it.value()->handleChannelClose();
        removeChannel(it);
    }
}

void SshChannelManager::handleRequestSuccess(const SshIncomingPacket &packet)
{
    if (m_waitingForwardServers.isEmpty()) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
                                 "Unexpected request success packet.",
                                 tr("Unexpected request success packet."));
    }
    SshTcpIpForwardServer::Ptr server = m_waitingForwardServers.takeFirst();
    if (server->state() == SshTcpIpForwardServer::Closing) {
        server->setClosed();
    } else if (server->state() == SshTcpIpForwardServer::Initializing) {
        quint16 port = server->port();
        if (port == 0)
            port = packet.extractRequestSuccess().bindPort;
        server->setListening(port);
        m_listeningForwardServers.append(server);
    } else {
        QSSH_ASSERT(false);
    }
}

void SshChannelManager::handleRequestFailure(const SshIncomingPacket &packet)
{
    Q_UNUSED(packet);
    if (m_waitingForwardServers.isEmpty()) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
                                 "Unexpected request failure packet.",
                                 tr("Unexpected request failure packet."));
    }
    SshTcpIpForwardServer::Ptr tunnel = m_waitingForwardServers.takeFirst();
    tunnel->setClosed();
}

SshChannelManager::ChannelIterator SshChannelManager::lookupChannelAsIterator(quint32 channelId,
    bool allowNotFound)
{
    ChannelIterator it = m_channels.find(channelId);
    if (it == m_channels.end() && !allowNotFound) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid channel id.",
            tr("Invalid channel id %1").arg(channelId));
    }
    return it;
}

AbstractSshChannel *SshChannelManager::lookupChannel(quint32 channelId,
    bool allowNotFound)
{
    ChannelIterator it = lookupChannelAsIterator(channelId, allowNotFound);
    return it == m_channels.end() ? 0 : it.value();
}

QSsh::SshRemoteProcess::Ptr SshChannelManager::createRemoteProcess(const QByteArray &command)
{
    SshRemoteProcess::Ptr proc(new SshRemoteProcess(command, m_nextLocalChannelId++, m_sendFacility));
    insertChannel(proc->d, proc);
    return proc;
}

QSsh::SshRemoteProcess::Ptr SshChannelManager::createRemoteShell()
{
    SshRemoteProcess::Ptr proc(new SshRemoteProcess(m_nextLocalChannelId++, m_sendFacility));
    insertChannel(proc->d, proc);
    return proc;
}

QSsh::SftpChannel::Ptr SshChannelManager::createSftpChannel()
{
    SftpChannel::Ptr sftp(new SftpChannel(m_nextLocalChannelId++, m_sendFacility));
    insertChannel(sftp->d, sftp);
    return sftp;
}

SshDirectTcpIpTunnel::Ptr SshChannelManager::createDirectTunnel(const QString &originatingHost,
        quint16 originatingPort, const QString &remoteHost, quint16 remotePort)
{
    SshDirectTcpIpTunnel::Ptr tunnel(new SshDirectTcpIpTunnel(m_nextLocalChannelId++,
            originatingHost, originatingPort, remoteHost, remotePort, m_sendFacility));
    insertChannel(tunnel->d, tunnel);
    return tunnel;
}

SshTcpIpForwardServer::Ptr SshChannelManager::createForwardServer(const QString &remoteHost,
        quint16 remotePort)
{
    SshTcpIpForwardServer::Ptr server(new SshTcpIpForwardServer(remoteHost, remotePort,
                                                                m_sendFacility));
    connect(server.data(), &SshTcpIpForwardServer::stateChanged,
            this, [this, server](SshTcpIpForwardServer::State state) {
        switch (state) {
        case SshTcpIpForwardServer::Closing:
            m_listeningForwardServers.removeOne(server);
            // fall through
        case SshTcpIpForwardServer::Initializing:
            m_waitingForwardServers.append(server);
            break;
        case SshTcpIpForwardServer::Listening:
        case SshTcpIpForwardServer::Inactive:
            break;
        }
    });
    return server;
}

void SshChannelManager::insertChannel(AbstractSshChannel *priv,
    const QSharedPointer<QObject> &pub)
{
    connect(priv, &AbstractSshChannel::timeout, this, &SshChannelManager::timeout);
    m_channels.insert(priv->localChannelId(), priv);
    m_sessions.insert(priv, pub);
}

int SshChannelManager::closeAllChannels(CloseAllMode mode)
{
    int count = 0;
    for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
        AbstractSshChannel * const channel = it.value();
        QSSH_ASSERT(channel->channelState() != AbstractSshChannel::Closed);
        if (channel->channelState() != AbstractSshChannel::CloseRequested) {
            channel->closeChannel();
            ++count;
        }
    }
    if (mode == CloseAllAndReset) {
        m_channels.clear();
        m_sessions.clear();
    }
    return count;
}

int SshChannelManager::channelCount() const
{
    return m_channels.count();
}

void SshChannelManager::removeChannel(ChannelIterator it)
{
    if (it == m_channels.end()) {
        throw SshClientException(SshInternalError,
                QLatin1String("Internal error: Unexpected channel lookup failure"));
    }
    const int removeCount = m_sessions.remove(it.value());
    if (removeCount != 1) {
        throw SshClientException(SshInternalError,
                QString::fromLatin1("Internal error: Unexpected session count %1 for channel.")
                                 .arg(removeCount));
    }
    m_channels.erase(it);
}

} // namespace Internal
} // namespace QSsh
