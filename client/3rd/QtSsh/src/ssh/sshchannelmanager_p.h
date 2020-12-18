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

#include <QHash>
#include <QObject>
#include <QSharedPointer>

namespace QSsh {
class SftpChannel;
class SshDirectTcpIpTunnel;
class SshRemoteProcess;
class SshTcpIpForwardServer;

namespace Internal {

class AbstractSshChannel;
class SshIncomingPacket;
class SshSendFacility;

class SshChannelManager : public QObject
{
    Q_OBJECT
public:
    SshChannelManager(SshSendFacility &sendFacility, QObject *parent);

    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SshRemoteProcess> createRemoteShell();
    QSharedPointer<SftpChannel> createSftpChannel();
    QSharedPointer<SshDirectTcpIpTunnel> createDirectTunnel(const QString &originatingHost,
            quint16 originatingPort, const QString &remoteHost, quint16 remotePort);
    QSharedPointer<SshTcpIpForwardServer> createForwardServer(const QString &remoteHost,
            quint16 remotePort);

    int channelCount() const;
    enum CloseAllMode { CloseAllRegular, CloseAllAndReset };
    int closeAllChannels(CloseAllMode mode);

    void handleChannelRequest(const SshIncomingPacket &packet);
    void handleChannelOpen(const SshIncomingPacket &packet);
    void handleChannelOpenFailure(const SshIncomingPacket &packet);
    void handleChannelOpenConfirmation(const SshIncomingPacket &packet);
    void handleChannelSuccess(const SshIncomingPacket &packet);
    void handleChannelFailure(const SshIncomingPacket &packet);
    void handleChannelWindowAdjust(const SshIncomingPacket &packet);
    void handleChannelData(const SshIncomingPacket &packet);
    void handleChannelExtendedData(const SshIncomingPacket &packet);
    void handleChannelEof(const SshIncomingPacket &packet);
    void handleChannelClose(const SshIncomingPacket &packet);
    void handleRequestSuccess(const SshIncomingPacket &packet);
    void handleRequestFailure(const SshIncomingPacket &packet);

signals:
    void timeout();

private:
    typedef QHash<quint32, AbstractSshChannel *>::Iterator ChannelIterator;

    ChannelIterator lookupChannelAsIterator(quint32 channelId,
        bool allowNotFound = false);
    AbstractSshChannel *lookupChannel(quint32 channelId,
        bool allowNotFound = false);
    void removeChannel(ChannelIterator it);
    void insertChannel(AbstractSshChannel *priv,
        const QSharedPointer<QObject> &pub);

    SshSendFacility &m_sendFacility;
    QHash<quint32, AbstractSshChannel *> m_channels;
    QHash<AbstractSshChannel *, QSharedPointer<QObject> > m_sessions;
    quint32 m_nextLocalChannelId;
    QList<QSharedPointer<SshTcpIpForwardServer>> m_waitingForwardServers;
    QList<QSharedPointer<SshTcpIpForwardServer>> m_listeningForwardServers;
};

} // namespace Internal
} // namespace QSsh
