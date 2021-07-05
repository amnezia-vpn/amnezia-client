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

#include "sshforwardedtcpiptunnel.h"
#include "sshforwardedtcpiptunnel_p.h"
#include "sshlogging_p.h"
#include "sshsendfacility_p.h"

namespace QSsh {

namespace Internal {
SshForwardedTcpIpTunnelPrivate::SshForwardedTcpIpTunnelPrivate(quint32 channelId,
                                                               SshSendFacility &sendFacility) :
    SshTcpIpTunnelPrivate(channelId, sendFacility)
{
    setChannelState(SessionRequested);
}

void SshForwardedTcpIpTunnelPrivate::handleOpenSuccessInternal()
{
    QSSH_ASSERT_AND_RETURN(channelState() == AbstractSshChannel::SessionEstablished);

    try {
        m_sendFacility.sendChannelOpenConfirmationPacket(remoteChannel(), localChannelId(),
                                                         initialWindowSize(), maxPacketSize());
    } catch (const std::exception &e) { // Won't happen, but let's play it safe.
        qCWarning(sshLog, "Botan error: %s", e.what());
        closeChannel();
    }
}

} // namespace Internal

using namespace Internal;

SshForwardedTcpIpTunnel::SshForwardedTcpIpTunnel(quint32 channelId, SshSendFacility &sendFacility) :
    d(new SshForwardedTcpIpTunnelPrivate(channelId, sendFacility))
{
    d->init(this);
}

SshForwardedTcpIpTunnel::~SshForwardedTcpIpTunnel()
{
    delete d;
}

bool SshForwardedTcpIpTunnel::atEnd() const
{
    return QIODevice::atEnd() && d->m_data.isEmpty();
}

qint64 SshForwardedTcpIpTunnel::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + d->m_data.count();
}

bool SshForwardedTcpIpTunnel::canReadLine() const
{
    return QIODevice::canReadLine() || d->m_data.contains('\n');
}

void SshForwardedTcpIpTunnel::close()
{
    d->closeChannel();
    QIODevice::close();
}

qint64 SshForwardedTcpIpTunnel::readData(char *data, qint64 maxlen)
{
    return d->readData(data, maxlen);
}

qint64 SshForwardedTcpIpTunnel::writeData(const char *data, qint64 len)
{
    return d->writeData(data, len);
}

} // namespace QSsh
