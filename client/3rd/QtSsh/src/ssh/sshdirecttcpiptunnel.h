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

#ifndef  SSHDIRECTTCPIPTUNNEL_H
#define  SSHDIRECTTCPIPTUNNEL_H

#include "ssh_global.h"

#include <QIODevice>
#include <QSharedPointer>

namespace QSsh {

namespace Internal {
class SshChannelManager;
class SshDirectTcpIpTunnelPrivate;
class SshSendFacility;
class SshTcpIpTunnelPrivate;
} // namespace Internal

class QSSH_EXPORT SshDirectTcpIpTunnel : public QIODevice
{
    Q_OBJECT

    friend class Internal::SshChannelManager;
    friend class Internal::SshTcpIpTunnelPrivate;

public:
    typedef QSharedPointer<SshDirectTcpIpTunnel> Ptr;

    ~SshDirectTcpIpTunnel();

    // QIODevice stuff
    bool atEnd() const;
    qint64 bytesAvailable() const;
    bool canReadLine() const;
    void close();
    bool isSequential() const { return true; }

    void initialize();

signals:
    void initialized();
    void error(const QString &reason);

private:
    SshDirectTcpIpTunnel(quint32 channelId, const QString &originatingHost,
            quint16 originatingPort, const QString &remoteHost, quint16 remotePort,
            Internal::SshSendFacility &sendFacility);

    // QIODevice stuff
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

    Internal::SshDirectTcpIpTunnelPrivate * const d;
};

} // namespace QSsh

#endif // SSHDIRECTTCPIPTUNNEL_H
