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

#include "ssh_global.h"
#include "sshforwardedtcpiptunnel.h"
#include <QObject>

namespace QSsh {

namespace Internal {
class SshChannelManager;
class SshTcpIpForwardServerPrivate;
class SshSendFacility;
class SshConnectionPrivate;
} // namespace Internal

class QSSH_EXPORT SshTcpIpForwardServer : public QObject
{
    Q_OBJECT
    friend class Internal::SshChannelManager;
    friend class Internal::SshConnectionPrivate;

public:
    enum State {
        Inactive,
        Initializing,
        Listening,
        Closing
    };

    typedef QSharedPointer<SshTcpIpForwardServer> Ptr;
    ~SshTcpIpForwardServer();

    const QString &bindAddress() const;
    quint16 port() const;
    State state() const;
    void initialize();
    void close();

    SshForwardedTcpIpTunnel::Ptr nextPendingConnection();

signals:
    void error(const QString &reason);
    void newConnection();
    void stateChanged(State state);

private:
    SshTcpIpForwardServer(const QString &bindAddress, quint16 bindPort,
                          Internal::SshSendFacility &sendFacility);
    void setListening(quint16 port);
    void setClosed();
    void setNewConnection(const SshForwardedTcpIpTunnel::Ptr &connection);

    Internal::SshTcpIpForwardServerPrivate * const d;
};

} // namespace QSsh
