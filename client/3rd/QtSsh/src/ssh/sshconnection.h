/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef SSHCONNECTION_H
#define SSHCONNECTION_H

#include "ssherrors.h"
#include "sshhostkeydatabase.h"

#include "ssh_global.h"

#include <QByteArray>
#include <QFlags>
#include <QMetaType>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QHostAddress>
#include <QUrl>

namespace QSsh {
class SftpChannel;
class SshDirectTcpIpTunnel;
class SshRemoteProcess;
class SshTcpIpForwardServer;

namespace Internal {
class SshConnectionPrivate;
} // namespace Internal

/*!
 * \brief Flags that control various general behavior
 */
enum SshConnectionOption {
    /// Set this to ignore the system defined proxy
    SshIgnoreDefaultProxy = 0x1,

    /// Fail instead of warn if the remote host violates the standard
    SshEnableStrictConformanceChecks = 0x2,

    /// Set the QAbstractSocket::LowDelayOption, which is the same as TCP_NODELAY
    SshLowDelaySocket = 0x4
};

Q_DECLARE_FLAGS(SshConnectionOptions, SshConnectionOption)

/*!
 * \brief How strict to be when checking the remote key
 */
enum SshHostKeyCheckingMode {
    /// Ignore the remote key
    SshHostKeyCheckingNone,

    /// Fail connection if either there is no key stored for this host or the key is not the same as earlier
    SshHostKeyCheckingStrict,

    /// Allow connecting if there is no stored key for the host, but fail if the key has changed
    SshHostKeyCheckingAllowNoMatch,

    /// Continue connection if the key doesn't match the stored key for the host
    SshHostKeyCheckingAllowMismatch
};

/*!
 * \brief Class to use to specify parameters used during connection.
 */
class QSSH_EXPORT SshConnectionParameters
{
public:

    /*!
     * \brief What kinds of authentication to attempt
     */
    enum AuthenticationType {
        AuthenticationTypePassword, ///< Only attempt to connect using the password set with setPassword().
        AuthenticationTypePublicKey, ///< Only attempt to authenticate with public key

        /// Only attempt keyboard interactive authentication.
        /// For now this only changes what to send to the server,
        /// we will still just try to use the password set here.
        AuthenticationTypeKeyboardInteractive,

        /// Any method using the password set with setPassword().
        /// Some servers disable \a "password", others disable \a "keyboard-interactive"
        AuthenticationTypeTryAllPasswordBasedMethods,

        /// ssh-agent authentication only
        AuthenticationTypeAgent,
    };

    SshConnectionParameters();

    /*!
     * \brief Returns the hostname or IP set with setHost()
     */
    QString host() const { return url.host(); }

    /*!
     * \brief Returns the port set with setPort()
     */
    int port() const { return url.port(); }

    /*!
     * \brief Returns the username set with setUsername()
     * \return
     */
    QString userName() const { return url.userName(); }

    /*!
     * \brief Returns the password set with setPassword()
     */
    QString password() const { return url.password(); }

    /*!
     * \brief Sets the hostname or IP to connect to
     * \param host The remote host
     */
    void setHost(const QString &host) { url.setHost(host); }

    /*!
     * \brief Sets the remote port to use
     * \param port
     */
    void setPort(int port) { url.setPort(port); }

    /*!
     * \brief Sets the username to use
     * \param name Username
     */
    void setUserName(const QString &name) { url.setUserName(name); }

    /*!
     * \brief Sets the password to attempt to use
     * \param password
     */
    void setPassword(const QString &password) { url.setPassword(password); }

    QUrl url;
    QString privateKeyFile;
    int timeout; // In seconds.
    AuthenticationType authenticationType;
    SshConnectionOptions options;
    SshHostKeyCheckingMode hostKeyCheckingMode;
    SshHostKeyDatabasePtr hostKeyDatabase;
};

/// @cond
QSSH_EXPORT bool operator==(const SshConnectionParameters &p1, const SshConnectionParameters &p2);
QSSH_EXPORT bool operator!=(const SshConnectionParameters &p1, const SshConnectionParameters &p2);
/// @endcond

/*!
 * \brief Network connection info.
 */
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


/*!
    \class QSsh::SshConnection

    \brief This class provides an SSH connection, implementing protocol version 2.0

    See acquireConnection() which provides a pool mechanism for re-use.

    It can spawn channels for remote execution and SFTP operations (version 3).
    It operates asynchronously (non-blocking) and is not thread-safe.
*/

class QSSH_EXPORT SshConnection : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief The current state of a connection
     */
    enum State { Unconnected, Connecting, Connected };

    /*!
     * \param serverInfo serverInfo connection parameters
     * \param parent Parent object.
     */
    explicit SshConnection(const SshConnectionParameters &serverInfo, QObject *parent = nullptr);

    void connectToHost();
    void disconnectFromHost();

    /*!
     * \brief Current state of this connection
     */
    State state() const;

    /*!
     * \brief Returns the error state of the connection
     * \returns If there is no error, returns \ref SshNoError if the connection is OK
     */
    SshError errorState() const;
    QString errorString() const;
    SshConnectionParameters connectionParameters() const;
    SshConnectionInfo connectionInfo() const;
    ~SshConnection();

    /*!
     * \brief Use this to launch remote commands
     * \param command The command to execute
     */
    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);

    /*!
     * \brief Creates a remote interactive session with a shell
     */
    QSharedPointer<SshRemoteProcess> createRemoteShell();
    QSharedPointer<SftpChannel> createSftpChannel();
    QSharedPointer<SshDirectTcpIpTunnel> createDirectTunnel(const QString &originatingHost,
            quint16 originatingPort, const QString &remoteHost, quint16 remotePort);
    QSharedPointer<SshTcpIpForwardServer> createForwardServer(const QString &remoteHost,
            quint16 remotePort);

    // -1 if an error occurred, number of channels closed otherwise.
    int closeAllChannels();
    int channelCount() const;
    const QByteArray &hostKeyFingerprint() const;

    /*!
     * \brief The X11 display name used for X11 forwarding
     * \return The name of the X11 display set for this connection
     */
    QString x11DisplayName() const;

signals:
    /*!
     * \brief Emitted when ready for use
     */
    void connected();

    /*!
     * \brief Emitted when the connection has been closed
     */
    void disconnected();

    /*!
     * \brief Emitted when data has been received
     * \param message The content of the data, same as the output you would get when running \a ssh on the command line
     */
    void dataAvailable(const QString &message);

    /*!
     * \brief Emitted when an error occured
     */
    void error(QSsh::SshError);

private:
    Internal::SshConnectionPrivate *d;
};

} // namespace QSsh

Q_DECLARE_METATYPE(QSsh::SshConnectionParameters::AuthenticationType)

#endif // SSHCONNECTION_H
