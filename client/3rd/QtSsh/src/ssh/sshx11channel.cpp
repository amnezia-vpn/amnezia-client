/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "sshx11channel_p.h"

#include "sshincomingpacket_p.h"
#include "sshlogging_p.h"
#include "sshsendfacility_p.h"

#include <QFileInfo>
#include <QLocalSocket>
#include <QTcpSocket>

namespace QSsh {
namespace Internal {

class X11Socket : public QObject
{
    Q_OBJECT
public:
    X11Socket(QObject *parent) : QObject(parent) { }

    void establishConnection(const X11DisplayInfo &displayInfo)
    {
        const bool hostNameIsPath = displayInfo.hostName.startsWith(QLatin1Char('/')); // macOS
        const bool hasActualHostName = !displayInfo.hostName.isEmpty()
                && displayInfo.hostName != QLatin1String("unix") && !displayInfo.hostName.endsWith(QLatin1String("/unix"))
                && !hostNameIsPath;
        if (hasActualHostName) {
            QTcpSocket * const socket = new QTcpSocket(this);
            connect(socket, &QTcpSocket::connected, this, &X11Socket::connected);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [this, socket] {
                emit error(socket->errorString());
            });
#else
            connect(socket, &QTcpSocket::errorOccurred, this, [this, socket] {
                emit error(socket->errorString());
            });
#endif

            socket->connectToHost(displayInfo.hostName, 6000 + displayInfo.display);
            m_socket = socket;
        } else {
            const QString serverBasePath = hostNameIsPath ? QString(displayInfo.hostName + QLatin1Char(':'))
                                                          : QLatin1String("/tmp/.X11-unix/X");
            QLocalSocket * const socket = new QLocalSocket(this);
            connect(socket, &QLocalSocket::connected, this, &X11Socket::connected);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            connect(socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error), this, [this, socket] {
                emit error(socket->errorString());
            });
#else
            connect(socket, &QLocalSocket::errorOccurred, this, [this, socket] {
                emit error(socket->errorString());
            });
#endif
            socket->connectToServer(serverBasePath + QString::number(displayInfo.display));
            m_socket = socket;
        }
        connect(m_socket, &QIODevice::readyRead, this,
                [this] { emit dataAvailable(m_socket->readAll()); });
    }

    void closeConnection()
    {
        m_socket->disconnect();
        if (localSocket())
            localSocket()->disconnectFromServer();
        else
            tcpSocket()->disconnectFromHost();
    }

    void write(const QByteArray &data)
    {
        m_socket->write(data);
    }

    bool hasError() const
    {
        return (localSocket() && localSocket()->error() != QLocalSocket::UnknownSocketError)
                || (tcpSocket() && tcpSocket()->error() != QTcpSocket::UnknownSocketError);
    }

    bool isConnected() const
    {
        return (localSocket() && localSocket()->state() == QLocalSocket::ConnectedState)
                || (tcpSocket() && tcpSocket()->state() == QTcpSocket::ConnectedState);
    }

signals:
    void connected();
    void error(const QString &message);
    void dataAvailable(const QByteArray &data);

private:
    QLocalSocket *localSocket() const { return qobject_cast<QLocalSocket *>(m_socket); }
    QTcpSocket *tcpSocket() const { return qobject_cast<QTcpSocket *>(m_socket); }

    QIODevice *m_socket = nullptr;
};

SshX11Channel::SshX11Channel(const X11DisplayInfo &displayInfo, quint32 channelId,
                             SshSendFacility &sendFacility)
    : AbstractSshChannel (channelId, sendFacility),
      m_x11Socket(new X11Socket(this)),
      m_displayInfo(displayInfo)
{
    setChannelState(SessionRequested); // Invariant for parent class.
}

void SshX11Channel::handleChannelSuccess()
{
    qCWarning(sshLog) << "unexpected channel success message for X11 channel";
}

void SshX11Channel::handleChannelFailure()
{
    qCWarning(sshLog) << "unexpected channel failure message for X11 channel";
}

void SshX11Channel::handleOpenSuccessInternal()
{
    m_sendFacility.sendChannelOpenConfirmationPacket(remoteChannel(), localChannelId(),
                                                     initialWindowSize(), maxPacketSize());
    connect(m_x11Socket, &X11Socket::connected, this,
        [this] {
            qCDebug(sshLog) << "x11 socket connected for channel" << localChannelId();
            if (!m_queuedRemoteData.isEmpty())
                handleRemoteData(QByteArray());
        }
    );
    connect(m_x11Socket, &X11Socket::error, this,
        [this](const QString &msg) {
            emit error(tr("X11 socket error: %1").arg(msg));
        }
    );
    connect(m_x11Socket, &X11Socket::dataAvailable, this,
        [this](const QByteArray &data) {
            qCDebug(sshLog) << "sending " << data.size() << "bytes from x11 socket to remote side "
                               "in channel" << localChannelId();
            sendData(data);
    });
    m_x11Socket->establishConnection(m_displayInfo);
}

void SshX11Channel::handleOpenFailureInternal(const QString &reason)
{
    qCWarning(sshLog) << "unexpected channel open failure message for X11 channel:" << reason;
}

void SshX11Channel::handleChannelDataInternal(const QByteArray &data)
{
    handleRemoteData(data);
}

void SshX11Channel::handleChannelExtendedDataInternal(quint32 type, const QByteArray &data)
{
    qCWarning(sshLog) << "unexpected extended data for X11 channel" << type << data;
}

void SshX11Channel::handleExitStatus(const SshChannelExitStatus &exitStatus)
{
    qCWarning(sshLog) << "unexpected exit status message on X11 channel" << exitStatus.exitStatus;
    closeChannel();
}

void SshX11Channel::handleExitSignal(const SshChannelExitSignal &signal)
{
    qCWarning(sshLog) << "unexpected exit signal message on X11 channel" << signal.error;
    closeChannel();
}

void SshX11Channel::closeHook()
{
    m_x11Socket->disconnect();
    m_x11Socket->closeConnection();
}

void SshX11Channel::handleRemoteData(const QByteArray &data)
{
    if (m_x11Socket->hasError())
        return;
    qCDebug(sshLog) << "received" << data.size() << "bytes from remote side in x11 channel"
                    << localChannelId();
    if (!m_x11Socket->isConnected()) {
        qCDebug(sshLog) << "x11 socket not yet connected, queueing data";
        m_queuedRemoteData += data;
        return;
    }
    if (m_haveReplacedRandomCookie) {
        qCDebug(sshLog) << "forwarding data to x11 socket";
        m_x11Socket->write(data);
        return;
    }
    m_queuedRemoteData += data;
    const int randomCookieOffset = m_queuedRemoteData.indexOf(m_displayInfo.randomCookie);
    if (randomCookieOffset == -1) {
        qCDebug(sshLog) << "random cookie has not appeared in remote data yet, queueing data";
        return;
    }
    m_queuedRemoteData.replace(randomCookieOffset, m_displayInfo.cookie.size(),
                               m_displayInfo.cookie);
    qCDebug(sshLog) << "found and replaced random cookie, forwarding data to x11 socket";
    m_x11Socket->write(m_queuedRemoteData);
    m_queuedRemoteData.clear();
    m_haveReplacedRandomCookie = true;
}

} // namespace Internal
} // namespace QSsh

#include <sshx11channel.moc>
