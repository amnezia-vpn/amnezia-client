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

#include "sshsendfacility_p.h"
#include "sshtcpiptunnel_p.h"

#include "sshincomingpacket_p.h"
#include "sshexception_p.h"
#include "sshlogging_p.h"

namespace QSsh {

namespace Internal {
SshTcpIpTunnelPrivate::SshTcpIpTunnelPrivate(quint32 channelId, SshSendFacility &sendFacility)
    : AbstractSshChannel(channelId, sendFacility)
{
    connect(this, &AbstractSshChannel::eof, this, &SshTcpIpTunnelPrivate::handleEof);
}

SshTcpIpTunnelPrivate::~SshTcpIpTunnelPrivate()
{
    closeChannel();
}



void SshTcpIpTunnelPrivate::handleChannelSuccess()
{
    throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_SUCCESS message.");
}

void SshTcpIpTunnelPrivate::handleChannelFailure()
{
    throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_FAILURE message.");
}

void SshTcpIpTunnelPrivate::handleOpenFailureInternal(const QString &reason)
{
    emit error(reason);
    closeChannel();
}

void SshTcpIpTunnelPrivate::handleChannelDataInternal(const QByteArray &data)
{
    m_data += data;
    emit readyRead();
}

void SshTcpIpTunnelPrivate::handleChannelExtendedDataInternal(quint32 type,
                                                                           const QByteArray &data)
{
    qCWarning(sshLog, "%s: Unexpected extended channel data. Type is %u, content is '%s'.",
              Q_FUNC_INFO, type, data.constData());
}

void SshTcpIpTunnelPrivate::handleExitStatus(const SshChannelExitStatus &exitStatus)
{
    qCWarning(sshLog, "%s: Unexpected exit status %d.", Q_FUNC_INFO, exitStatus.exitStatus);
}

void SshTcpIpTunnelPrivate::handleExitSignal(const SshChannelExitSignal &signal)
{
    qCWarning(sshLog, "%s: Unexpected exit signal %s.", Q_FUNC_INFO, signal.signal.constData());
}

void SshTcpIpTunnelPrivate::closeHook()
{
    emit closed();
}

void SshTcpIpTunnelPrivate::handleEof()
{
    /*
     * For some reason, the OpenSSH server only sends EOF when the remote port goes away,
     * but does not close the channel, even though it becomes useless in that case.
     * So we close it ourselves.
     */
    closeChannel();
}

qint64 SshTcpIpTunnelPrivate::readData(char *data, qint64 maxlen)
{
    const qint64 bytesRead = qMin(qint64(m_data.count()), maxlen);
    memcpy(data, m_data.constData(), bytesRead);
    m_data.remove(0, bytesRead);
    return bytesRead;
}

qint64 SshTcpIpTunnelPrivate::writeData(const char *data, qint64 len)
{
    QSSH_ASSERT_AND_RETURN_VALUE(channelState() == AbstractSshChannel::SessionEstablished, 0);

    sendData(QByteArray(data, len));
    return len;
}

} // namespace Internal

} // namespace QSsh
