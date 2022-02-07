/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
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
        d->m_sendFacility.sendDirectTcpIpPacket(d->localChannelId(), SshDirectTcpIpTunnelPrivate::initialWindowSize(),
            SshDirectTcpIpTunnelPrivate::maxPacketSize(), d->m_remoteHost.toUtf8(), d->m_remotePort,
            d->m_originatingHost.toUtf8(), d->m_originatingPort);
        d->setChannelState(AbstractSshChannel::SessionRequested);
        d->m_timeoutTimer.setTimerType(Qt::VeryCoarseTimer);
        d->m_timeoutTimer.start(SshDirectTcpIpTunnelPrivate::ReplyTimeout);
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
