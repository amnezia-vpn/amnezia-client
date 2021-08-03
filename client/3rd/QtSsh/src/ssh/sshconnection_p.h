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

#ifndef SSHCONNECTION_P_H
#define SSHCONNECTION_P_H

#include "sshconnection.h"
#include "sshexception_p.h"
#include "sshincomingpacket_p.h"
#include "sshsendfacility_p.h"

#include <QHash>
#include <QList>
#include <QQueue>
#include <QObject>
#include <QPair>
#include <QScopedPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

namespace QSsh {
class SftpChannel;
class SshRemoteProcess;
class SshDirectTcpIpTunnel;
class SshTcpIpForwardServer;

namespace Internal {
class SshChannelManager;

// NOTE: When you add stuff here, don't forget to update m_packetHandlers.
enum SshStateInternal {
    SocketUnconnected, // initial and after disconnect
    SocketConnecting, // After connectToHost()
    SocketConnected, // After socket's connected() signal
    UserAuthServiceRequested,
    WaitingForAgentKeys,
    UserAuthRequested,
    ConnectionEstablished // After service has been started
    // ...
};

enum SshKeyExchangeState {
    NoKeyExchange,
    KexInitSent,
    DhInitSent,
    NewKeysSent,
    KeyExchangeSuccess  // After server's DH_REPLY message
};

class SshConnectionPrivate : public QObject
{
    Q_OBJECT
    friend class QSsh::SshConnection;
public:
    SshConnectionPrivate(SshConnection *conn,
        const SshConnectionParameters &serverInfo);
    ~SshConnectionPrivate();

    void connectToHost();
    void closeConnection(SshErrorCode sshError, SshError userError,
        const QByteArray &serverErrorString, const QString &userErrorString);
    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SshRemoteProcess> createRemoteShell();
    QSharedPointer<SftpChannel> createSftpChannel();
    QSharedPointer<SshDirectTcpIpTunnel> createDirectTunnel(const QString &originatingHost,
            quint16 originatingPort, const QString &remoteHost, quint16 remotePort);
    QSharedPointer<SshTcpIpForwardServer> createForwardServer(const QString &remoteHost,
            quint16 remotePort);

    SshStateInternal state() const { return m_state; }
    SshError errorState() const { return m_error; }
    QString errorString() const { return m_errorString; }
    const QByteArray &hostKeyFingerprint() const { return m_hostFingerprint; }

signals:
    void connected();
    void disconnected();
    void dataAvailable(const QString &message);
    void error(QSsh::SshError);

private:
    void handleSocketConnected();
    void handleIncomingData();
    void handleSocketError();
    void handleSocketDisconnected();
    void handleTimeout();
    void sendKeepAlivePacket();

    void handleAgentKeysUpdated();
    void handleSignatureFromAgent(const QByteArray &key, const QByteArray &signature, uint token);
    void tryAllAgentKeys();
    void authenticateWithPublicKey();
    void setAgentError();

    void handleServerId();
    void handlePackets();
    void handleCurrentPacket();
    void handleKeyExchangeInitPacket();
    void handleKeyExchangeReplyPacket();
    void handleNewKeysPacket();
    void handleServiceAcceptPacket();
    void handlePasswordExpiredPacket();
    void handleUserAuthInfoRequestPacket();
    void handleUserAuthSuccessPacket();
    void handleUserAuthFailurePacket();
    void handleUserAuthKeyOkPacket();
    void handleUserAuthBannerPacket();
    void handleUnexpectedPacket();
    void handleGlobalRequest();
    void handleDebugPacket();
    void handleUnimplementedPacket();
    void handleChannelRequest();
    void handleChannelOpen();
    void handleChannelOpenFailure();
    void handleChannelOpenConfirmation();
    void handleChannelSuccess();
    void handleChannelFailure();
    void handleChannelWindowAdjust();
    void handleChannelData();
    void handleChannelExtendedData();
    void handleChannelEof();
    void handleChannelClose();
    void handleDisconnect();
    void handleRequestSuccess();
    void handleRequestFailure();

    bool canUseSocket() const;
    void createPrivateKey();

    void sendData(const QByteArray &data);

    uint tokenForAgent() const;

    typedef void (SshConnectionPrivate::*PacketHandler)();
    typedef QList<SshStateInternal> StateList;
    void setupPacketHandlers();
    void setupPacketHandler(SshPacketType type, const StateList &states,
        PacketHandler handler);

    typedef QPair<StateList, PacketHandler> HandlerInStates;
    QHash<SshPacketType, HandlerInStates> m_packetHandlers;

    static const quint64 InvalidSeqNr;

    QTcpSocket *m_socket;
    SshStateInternal m_state;
    SshKeyExchangeState m_keyExchangeState;
    SshIncomingPacket m_incomingPacket;
    SshSendFacility m_sendFacility;
    SshChannelManager * const m_channelManager;
    const SshConnectionParameters m_connParams;
    QByteArray m_incomingData;
    SshError m_error;
    QString m_errorString;
    QScopedPointer<SshKeyExchange> m_keyExchange;
    QByteArray m_hostFingerprint;
    QTimer m_timeoutTimer;
    QTimer m_keepAliveTimer;
    bool m_ignoreNextPacket;
    SshConnection *m_conn;
    quint64 m_lastInvalidMsgSeqNr;
    QByteArray m_serverId;
    QByteArray m_agentSignature;
    QQueue<QByteArray> m_pendingKeyChecks;
    QByteArray m_agentKeyToUse;
    bool m_serverHasSentDataBeforeId;
    bool m_triedAllPasswordBasedMethods;
    bool m_agentKeysUpToDate;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHCONNECTION_P_H
