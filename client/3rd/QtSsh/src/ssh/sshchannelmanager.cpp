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
#include "sshx11channel_p.h"
#include "sshx11inforetriever_p.h"

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
    const SshChannelOpenGeneric channelOpen = packet.extractChannelOpen();
    if (channelOpen.channelType == SshIncomingPacket::ForwardedTcpIpType) {
        handleChannelOpenForwardedTcpIp(channelOpen);
        return;
    }
    if (channelOpen.channelType == "x11") {
        handleChannelOpenX11(channelOpen);
        return;
    }
    try {
        m_sendFacility.sendChannelOpenFailurePacket(channelOpen.commonData.remoteChannel,
                                                    SSH_OPEN_UNKNOWN_CHANNEL_TYPE, QByteArray());
    }  catch (const std::exception &e) {
        qCWarning(sshLog, "Botan error: %s", e.what());
    }
}

void SshChannelManager::handleChannelOpenFailure(const SshIncomingPacket &packet)
{
   const SshChannelOpenFailure &failure = packet.extractChannelOpenFailure();
   ChannelIterator it = lookupChannelAsIterator(failure.localChannel);
   try {
       it.value()->handleOpenFailure(failure.reasonString);
   } catch (const SshServerException &e) {
       removeChannel(it);
       throw;
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
    return it == m_channels.end() ? nullptr : it.value();
}

QSsh::SshRemoteProcess::Ptr SshChannelManager::createRemoteProcess(const QByteArray &command)
{
    SshRemoteProcess::Ptr proc(new SshRemoteProcess(command, m_nextLocalChannelId++, m_sendFacility));
    insertChannel(proc->d, proc);
    connect(proc->d, &SshRemoteProcessPrivate::destroyed, this, [this] {
        m_x11ForwardingRequests.removeOne(static_cast<SshRemoteProcessPrivate *>(sender()));
    });
    connect(proc->d, &SshRemoteProcessPrivate::x11ForwardingRequested, this,
            [this, proc = proc->d](const QString &displayName) {
        if (!x11DisplayName().isEmpty()) {
            if (x11DisplayName() != displayName) {
                proc->failToStart(tr("Cannot forward to display %1 on SSH connection that is "
                                     "already forwarding to display %2.")
                                  .arg(displayName, x11DisplayName()));
                return;
            }
            if (!m_x11DisplayInfo.cookie.isEmpty())
                proc->startProcess(m_x11DisplayInfo);
            else
                m_x11ForwardingRequests << proc;
            return;
        }
        m_x11DisplayInfo.displayName = displayName;
        m_x11ForwardingRequests << proc;
        auto * const x11InfoRetriever = new SshX11InfoRetriever(displayName, this);
        const auto failureHandler = [this](const QString &errorMessage) {
            for (SshRemoteProcessPrivate * const proc : qAsConst(m_x11ForwardingRequests))
                proc->failToStart(errorMessage);
            m_x11ForwardingRequests.clear();
        };
        connect(x11InfoRetriever, &SshX11InfoRetriever::failure, this, failureHandler);
        const auto successHandler = [this](const X11DisplayInfo &displayInfo) {
            m_x11DisplayInfo = displayInfo;
            for (SshRemoteProcessPrivate * const proc : qAsConst(m_x11ForwardingRequests))
                proc->startProcess(displayInfo);
            m_x11ForwardingRequests.clear();
        };
        connect(x11InfoRetriever, &SshX11InfoRetriever::success, this, successHandler);
        qCDebug(sshLog) << "starting x11 info retriever";
        x11InfoRetriever->start();
    });
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
            Q_FALLTHROUGH();
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

void SshChannelManager::handleChannelOpenForwardedTcpIp(
        const SshChannelOpenGeneric &channelOpenGeneric)
{
    const SshChannelOpenForwardedTcpIp channelOpen
            = SshIncomingPacket::extractChannelOpenForwardedTcpIp(channelOpenGeneric);

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
        try {
            m_sendFacility.sendChannelOpenFailurePacket(channelOpen.common.remoteChannel,
                                                        SSH_OPEN_ADMINISTRATIVELY_PROHIBITED,
                                                        QByteArray());
        }  catch (const std::exception &e) {
            qCWarning(sshLog, "Botan error: %s", e.what());
        }
        return;
    }

    SshForwardedTcpIpTunnel::Ptr tunnel(new SshForwardedTcpIpTunnel(m_nextLocalChannelId++,
                                                                    m_sendFacility));
    tunnel->d->handleOpenSuccess(channelOpen.common.remoteChannel,
                                 channelOpen.common.remoteWindowSize,
                                 channelOpen.common.remoteMaxPacketSize);
    tunnel->open(QIODevice::ReadWrite);
    server->setNewConnection(tunnel);
    insertChannel(tunnel->d, tunnel);
}

void SshChannelManager::handleChannelOpenX11(const SshChannelOpenGeneric &channelOpenGeneric)
{
    qCDebug(sshLog) << "incoming X11 channel open request";
    const SshChannelOpenX11 channelOpen
            = SshIncomingPacket::extractChannelOpenX11(channelOpenGeneric);
    if (m_x11DisplayInfo.cookie.isEmpty()) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
                                   "Server attempted to open an unrequested X11 channel.");
    }
    SshX11Channel * const x11Channel = new SshX11Channel(m_x11DisplayInfo,
                                                         m_nextLocalChannelId++,
                                                         m_sendFacility);
    x11Channel->setParent(this);
    x11Channel->handleOpenSuccess(channelOpen.common.remoteChannel,
                                  channelOpen.common.remoteWindowSize,
                                  channelOpen.common.remoteMaxPacketSize);
    insertChannel(x11Channel, QSharedPointer<QObject>());
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
