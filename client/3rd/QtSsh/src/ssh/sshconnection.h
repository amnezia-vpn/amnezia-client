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

#include "ssherrors.h"
#include "sshhostkeydatabase.h"

#include "ssh_global.h"

#include <QByteArray>
#include <QFlags>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QHostAddress>

namespace QSsh {
class SftpChannel;
class SshDirectTcpIpTunnel;
class SshRemoteProcess;
class SshTcpIpForwardServer;

namespace Internal { class SshConnectionPrivate; }

enum SshConnectionOption {
    SshIgnoreDefaultProxy = 0x1,
    SshEnableStrictConformanceChecks = 0x2
};

Q_DECLARE_FLAGS(SshConnectionOptions, SshConnectionOption)

enum SshHostKeyCheckingMode {
    SshHostKeyCheckingNone,
    SshHostKeyCheckingStrict,
    SshHostKeyCheckingAllowNoMatch,
    SshHostKeyCheckingAllowMismatch
};

class QSSH_EXPORT SshConnectionParameters
{
public:
    enum AuthenticationType {
        AuthenticationTypePassword,
        AuthenticationTypePublicKey,
        AuthenticationTypeKeyboardInteractive,

        // Some servers disable "password", others disable "keyboard-interactive".
        AuthenticationTypeTryAllPasswordBasedMethods
    };

    SshConnectionParameters();

    QString host;
    QString userName;
    QString password;
    QString privateKeyFile;
    int timeout; // In seconds.
    AuthenticationType authenticationType;
    quint16 port;
    SshConnectionOptions options;
    SshHostKeyCheckingMode hostKeyCheckingMode;
    SshHostKeyDatabasePtr hostKeyDatabase;
};

QSSH_EXPORT bool operator==(const SshConnectionParameters &p1, const SshConnectionParameters &p2);
QSSH_EXPORT bool operator!=(const SshConnectionParameters &p1, const SshConnectionParameters &p2);

class QSSH_EXPORT SshConnectionInfo
{
public:
    SshConnectionInfo() : localPort(0), peerPort(0) {}
    SshConnectionInfo(const QHostAddress &la, quint16 lp, const QHostAddress &pa, quint16 pp)
        : localAddress(la), localPort(lp), peerAddress(pa), peerPort(pp) {}

    QHostAddress localAddress;
    quint16 localPort;
    QHostAddress peerAddress;
    quint16 peerPort;
};

class QSSH_EXPORT SshConnection : public QObject
{
    Q_OBJECT

public:
    enum State { Unconnected, Connecting, Connected };

    explicit SshConnection(const SshConnectionParameters &serverInfo, QObject *parent = 0);

    void connectToHost();
    void disconnectFromHost();
    State state() const;
    SshError errorState() const;
    QString errorString() const;
    SshConnectionParameters connectionParameters() const;
    SshConnectionInfo connectionInfo() const;
    ~SshConnection();

    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SshRemoteProcess> createRemoteShell();
    QSharedPointer<SftpChannel> createSftpChannel();
    QSharedPointer<SshDirectTcpIpTunnel> createDirectTunnel(const QString &originatingHost,
            quint16 originatingPort, const QString &remoteHost, quint16 remotePort);
    QSharedPointer<SshTcpIpForwardServer> createForwardServer(const QString &remoteHost,
            quint16 remotePort);

    // -1 if an error occurred, number of channels closed otherwise.
    int closeAllChannels();

    int channelCount() const;

signals:
    void connected();
    void disconnected();
    void dataAvailable(const QString &message);
    void error(QSsh::SshError);

private:
    Internal::SshConnectionPrivate *d;
};

} // namespace QSsh
