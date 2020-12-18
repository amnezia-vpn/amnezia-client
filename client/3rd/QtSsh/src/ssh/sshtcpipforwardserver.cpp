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

#include "sshtcpipforwardserver.h"
#include "sshtcpipforwardserver_p.h"

#include "sshlogging_p.h"
#include "sshsendfacility_p.h"

namespace QSsh {
namespace Internal {

SshTcpIpForwardServerPrivate::SshTcpIpForwardServerPrivate(const QString &bindAddress,
        quint16 bindPort, SshSendFacility &sendFacility)
    : m_sendFacility(sendFacility),
      m_bindAddress(bindAddress),
      m_bindPort(bindPort),
      m_state(SshTcpIpForwardServer::Inactive)
{
}

} // namespace Internal

using namespace Internal;

SshTcpIpForwardServer::SshTcpIpForwardServer(const QString &bindAddress, quint16 bindPort,
                                             SshSendFacility &sendFacility)
    : d(new SshTcpIpForwardServerPrivate(bindAddress, bindPort, sendFacility))
{
    connect(&d->m_timeoutTimer, &QTimer::timeout, this, &SshTcpIpForwardServer::setClosed);
}

void SshTcpIpForwardServer::setListening(quint16 port)
{
    QSSH_ASSERT_AND_RETURN(d->m_state != Listening);
    d->m_bindPort = port;
    d->m_state = Listening;
    emit stateChanged(Listening);
}

void SshTcpIpForwardServer::setClosed()
{
    QSSH_ASSERT_AND_RETURN(d->m_state != Inactive);
    d->m_state = Inactive;
    emit stateChanged(Inactive);
}

void SshTcpIpForwardServer::setNewConnection(const SshForwardedTcpIpTunnel::Ptr &connection)
{
    d->m_pendingConnections.append(connection);
    emit newConnection();
}

SshTcpIpForwardServer::~SshTcpIpForwardServer()
{
    delete d;
}

void SshTcpIpForwardServer::initialize()
{
    if (d->m_state == Inactive || d->m_state == Closing) {
        try {
            d->m_state = Initializing;
            emit stateChanged(Initializing);
            d->m_sendFacility.sendTcpIpForwardPacket(d->m_bindAddress.toUtf8(), d->m_bindPort);
            d->m_timeoutTimer.start(d->ReplyTimeout);
        } catch (const std::exception &e) {
            qCWarning(sshLog, "Botan error: %s", e.what());
            d->m_timeoutTimer.stop();
            setClosed();
        }
    }
}

void SshTcpIpForwardServer::close()
{
    d->m_timeoutTimer.stop();

    if (d->m_state == Initializing || d->m_state == Listening) {
        try {
            d->m_state = Closing;
            emit stateChanged(Closing);
            d->m_sendFacility.sendCancelTcpIpForwardPacket(d->m_bindAddress.toUtf8(),
                                                           d->m_bindPort);
            d->m_timeoutTimer.start(d->ReplyTimeout);
        } catch (const std::exception &e) {
            qCWarning(sshLog, "Botan error: %s", e.what());
            d->m_timeoutTimer.stop();
            setClosed();
        }
    }
}

const QString &SshTcpIpForwardServer::bindAddress() const
{
    return d->m_bindAddress;
}

quint16 SshTcpIpForwardServer::port() const
{
    return d->m_bindPort;
}

SshTcpIpForwardServer::State SshTcpIpForwardServer::state() const
{
    return d->m_state;
}

SshForwardedTcpIpTunnel::Ptr SshTcpIpForwardServer::nextPendingConnection()
{
    return d->m_pendingConnections.takeFirst();
}

} // namespace QSsh
