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

#include "sshdirecttcpiptunnel.h"
#include "sshdirecttcpiptunnel_p.h"

#include "sshincomingpacket_p.h"
#include "sshlogging_p.h"
#include "sshsendfacility_p.h"

#include <QTimer>

namespace QSsh {
namespace Internal {

SshDirectTcpIpTunnelPrivate::SshDirectTcpIpTunnelPrivate(quint32 channelId,
        const QString &originatingHost, quint16 originatingPort, const QString &remoteHost,
        quint16 remotePort, SshSendFacility &sendFacility)
    : SshTcpIpTunnelPrivate(channelId, sendFacility),
      m_originatingHost(originatingHost),
      m_originatingPort(originatingPort),
      m_remoteHost(remoteHost),
      m_remotePort(remotePort)
{
}

void SshDirectTcpIpTunnelPrivate::handleOpenSuccessInternal()
{
    emit initialized();
}

} // namespace Internal

using namespace Internal;

SshDirectTcpIpTunnel::SshDirectTcpIpTunnel(quint32 channelId, const QString &originatingHost,
        quint16 originatingPort, const QString &remoteHost, quint16 remotePort,
        SshSendFacility &sendFacility)
    : d(new SshDirectTcpIpTunnelPrivate(channelId, originatingHost, originatingPort, remoteHost,
                                        remotePort, sendFacility))
{
    d->init(this);
    connect(d, &SshDirectTcpIpTunnelPrivate::initialized,
            this, &SshDirectTcpIpTunnel::initialized, Qt::QueuedConnection);
}

SshDirectTcpIpTunnel::~SshDirectTcpIpTunnel()
{
    delete d;
}

bool SshDirectTcpIpTunnel::atEnd() const
{
    return QIODevice::atEnd() && d->m_data.isEmpty();
}

qint64 SshDirectTcpIpTunnel::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + d->m_data.count();
}

bool SshDirectTcpIpTunnel::canReadLine() const
{
    return QIODevice::canReadLine() || d->m_data.contains('\n');
}

void SshDirectTcpIpTunnel::close()
{
    d->closeChannel();
    QIODevice::close();
}

void SshDirectTcpIpTunnel::initialize()
{
    QSSH_ASSERT_AND_RETURN(d->channelState() == AbstractSshChannel::Inactive);

    try {
        QIODevice::open(QIODevice::ReadWrite);
        d->m_sendFacility.sendDirectTcpIpPacket(d->localChannelId(), d->initialWindowSize(),
            d->maxPacketSize(), d->m_remoteHost.toUtf8(), d->m_remotePort,
            d->m_originatingHost.toUtf8(), d->m_originatingPort);
        d->setChannelState(AbstractSshChannel::SessionRequested);
        d->m_timeoutTimer.start(d->ReplyTimeout);
    }  catch (const std::exception &e) { // Won't happen, but let's play it safe.
        qCWarning(sshLog, "Botan error: %s", e.what());
        d->closeChannel();
    }
}

qint64 SshDirectTcpIpTunnel::readData(char *data, qint64 maxlen)
{
    return d->readData(data, maxlen);
}

qint64 SshDirectTcpIpTunnel::writeData(const char *data, qint64 len)
{
    return d->writeData(data, len);
}

} // namespace QSsh
