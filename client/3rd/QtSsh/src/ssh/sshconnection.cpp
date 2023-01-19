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

#include "sshconnection.h"
#include "sshconnection_p.h"

#include "sftpchannel.h"
#include "sshagent_p.h"
#include "sshcapabilities_p.h"
#include "sshchannelmanager_p.h"
#include "sshcryptofacility_p.h"
#include "sshdirecttcpiptunnel.h"
#include "sshtcpipforwardserver.h"
#include "sshexception_p.h"
#include "sshkeyexchange_p.h"
#include "sshremoteprocess.h"
#include "sshlogging_p.h"

#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkProxy>
#include <QRegularExpression>
#include <QTcpSocket>

namespace QSsh {

namespace {
const QByteArray ClientId("SSH-2.0-QtCreator\r\n");
}

SshConnectionParameters::SshConnectionParameters() :
    timeout(0), authenticationType(AuthenticationTypePublicKey),
    hostKeyCheckingMode(SshHostKeyCheckingNone)
{
    url.setPort(0);
    options |= SshIgnoreDefaultProxy;
    hostKeyDatabase = SshHostKeyDatabasePtr(new SshHostKeyDatabase);
}

static inline bool equals(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return p1.url == p2.url
            && p1.authenticationType == p2.authenticationType
            && p1.privateKeyFile == p2.privateKeyFile
            && p1.hostKeyCheckingMode == p2.hostKeyCheckingMode
            && p1.timeout == p2.timeout;
}

bool operator==(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return equals(p1, p2);
}

bool operator!=(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return !equals(p1, p2);
}

SshConnection::SshConnection(const SshConnectionParameters &serverInfo, QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QSsh::SshError>("QSsh::SshError");
    qRegisterMetaType<QSsh::SftpJobId>("QSsh::SftpJobId");
    qRegisterMetaType<QSsh::SftpFileInfo>("QSsh::SftpFileInfo");
    qRegisterMetaType<QSsh::SftpError>("QSsh::SftpError");
    qRegisterMetaType<QSsh::SftpError>("SftpError");
    qRegisterMetaType<QList <QSsh::SftpFileInfo> >("QList<QSsh::SftpFileInfo>");

    d = new Internal::SshConnectionPrivate(this, serverInfo);
    connect(d, &Internal::SshConnectionPrivate::connected, this, &SshConnection::connected,
        Qt::QueuedConnection);
    connect(d, &Internal::SshConnectionPrivate::dataAvailable, this,
        &SshConnection::dataAvailable, Qt::QueuedConnection);
    connect(d, &Internal::SshConnectionPrivate::disconnected, this, &SshConnection::disconnected,
        Qt::QueuedConnection);
    connect(d, &Internal::SshConnectionPrivate::error, this,
            &SshConnection::error, Qt::QueuedConnection);
}

const QByteArray &SshConnection::hostKeyFingerprint() const
{
    return d->hostKeyFingerprint();
}

void SshConnection::connectToHost()
{
    d->connectToHost();
}

void SshConnection::disconnectFromHost()
{
    d->closeConnection(Internal::SSH_DISCONNECT_BY_APPLICATION, SshNoError, "",
        QString());
}

SshConnection::State SshConnection::state() const
{
    switch (d->state()) {
    case Internal::SocketUnconnected:
        return Unconnected;
    case Internal::ConnectionEstablished:
        return Connected;
    default:
        return Connecting;
    }
}

SshError SshConnection::errorState() const
{
    return d->errorState();
}

QString SshConnection::errorString() const
{
    return d->errorString();
}

SshConnectionParameters SshConnection::connectionParameters() const
{
    return d->m_connParams;
}

SshConnectionInfo SshConnection::connectionInfo() const
{
    QSSH_ASSERT_AND_RETURN_VALUE(state() == Connected, SshConnectionInfo());

    return SshConnectionInfo(d->m_socket->localAddress(), d->m_socket->localPort(),
        d->m_socket->peerAddress(), d->m_socket->peerPort());
}

SshConnection::~SshConnection()
{
    disconnect();
    disconnectFromHost();
    delete d;
}

QSharedPointer<SshRemoteProcess> SshConnection::createRemoteProcess(const QByteArray &command)
{
    QSSH_ASSERT_AND_RETURN_VALUE(state() == Connected, QSharedPointer<SshRemoteProcess>());
    return d->createRemoteProcess(command);
}

QSharedPointer<SshRemoteProcess> SshConnection::createRemoteShell()
{
    QSSH_ASSERT_AND_RETURN_VALUE(state() == Connected, QSharedPointer<SshRemoteProcess>());
    return d->createRemoteShell();
}

QSharedPointer<SftpChannel> SshConnection::createSftpChannel()
{
    QSSH_ASSERT_AND_RETURN_VALUE(state() == Connected, QSharedPointer<SftpChannel>());
    return d->createSftpChannel();
}

SshDirectTcpIpTunnel::Ptr SshConnection::createDirectTunnel(const QString &originatingHost,
        quint16 originatingPort, const QString &remoteHost, quint16 remotePort)
{
    QSSH_ASSERT_AND_RETURN_VALUE(state() == Connected, SshDirectTcpIpTunnel::Ptr());
    return d->createDirectTunnel(originatingHost, originatingPort, remoteHost, remotePort);
}

QSharedPointer<SshTcpIpForwardServer> SshConnection::createForwardServer(const QString &remoteHost,
        quint16 remotePort)
{
    QSSH_ASSERT_AND_RETURN_VALUE(state() == Connected, SshTcpIpForwardServer::Ptr());
    return d->createForwardServer(remoteHost, remotePort);
}

int SshConnection::closeAllChannels()
{
    try {
        return d->m_channelManager->closeAllChannels(Internal::SshChannelManager::CloseAllRegular);
    } catch (const std::exception &e) {
        qCWarning(Internal::sshLog, "%s: %s", Q_FUNC_INFO, e.what());
        return -1;
    }
}

int SshConnection::channelCount() const
{
    return d->m_channelManager->channelCount();
}

QString SshConnection::x11DisplayName() const
{
    return d->m_channelManager->x11DisplayName();
}

namespace Internal {

SshConnectionPrivate::SshConnectionPrivate(SshConnection *conn,
    const SshConnectionParameters &serverInfo)
    : m_socket(new QTcpSocket(this)), m_state(SocketUnconnected),
      m_sendFacility(m_socket),
      m_channelManager(new SshChannelManager(m_sendFacility, this)),
      m_connParams(serverInfo), m_error(SshNoError), m_ignoreNextPacket(false),
      m_conn(conn)
{
    setupPacketHandlers();

    if (m_connParams.options & SshLowDelaySocket) {
        m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    }

    m_socket->setProxy((m_connParams.options & SshIgnoreDefaultProxy)
            ? QNetworkProxy::NoProxy : QNetworkProxy::DefaultProxy);
    m_timeoutTimer.setTimerType(Qt::VeryCoarseTimer);
    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(m_connParams.timeout * 1000);
    m_keepAliveTimer.setTimerType(Qt::VeryCoarseTimer);
    m_keepAliveTimer.setSingleShot(true);
    m_keepAliveTimer.setInterval(10000);
    connect(m_channelManager, &SshChannelManager::timeout,
            this, &SshConnectionPrivate::handleTimeout);
}

SshConnectionPrivate::~SshConnectionPrivate()
{
    disconnect();
}

void SshConnectionPrivate::setupPacketHandlers()
{
    typedef SshConnectionPrivate This;

    setupPacketHandler(SSH_MSG_KEXINIT, StateList() << SocketConnected
        << ConnectionEstablished, &This::handleKeyExchangeInitPacket);
    setupPacketHandler(SSH_MSG_KEXDH_REPLY, StateList() << SocketConnected
        << ConnectionEstablished, &This::handleKeyExchangeReplyPacket);

    setupPacketHandler(SSH_MSG_NEWKEYS, StateList() << SocketConnected
        << ConnectionEstablished, &This::handleNewKeysPacket);
    setupPacketHandler(SSH_MSG_SERVICE_ACCEPT,
        StateList() << UserAuthServiceRequested,
        &This::handleServiceAcceptPacket);
    if (m_connParams.authenticationType == SshConnectionParameters::AuthenticationTypePassword
            || m_connParams.authenticationType == SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods) {
        setupPacketHandler(SSH_MSG_USERAUTH_PASSWD_CHANGEREQ,
                StateList() << UserAuthRequested, &This::handlePasswordExpiredPacket);
    }
    setupPacketHandler(SSH_MSG_GLOBAL_REQUEST,
        StateList() << ConnectionEstablished, &This::handleGlobalRequest);

    const StateList authReqList = StateList() << UserAuthRequested;
    setupPacketHandler(SSH_MSG_USERAUTH_BANNER, authReqList,
        &This::handleUserAuthBannerPacket);
    setupPacketHandler(SSH_MSG_USERAUTH_SUCCESS, authReqList,
        &This::handleUserAuthSuccessPacket);
    setupPacketHandler(SSH_MSG_USERAUTH_FAILURE, authReqList,
        &This::handleUserAuthFailurePacket);
    if (m_connParams.authenticationType == SshConnectionParameters::AuthenticationTypeKeyboardInteractive
            || m_connParams.authenticationType == SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods) {
        setupPacketHandler(SSH_MSG_USERAUTH_INFO_REQUEST, authReqList,
                &This::handleUserAuthInfoRequestPacket);
    }
    setupPacketHandler(SSH_MSG_USERAUTH_PK_OK, authReqList, &This::handleUserAuthKeyOkPacket);

    const StateList connectedList
        = StateList() << ConnectionEstablished;
    setupPacketHandler(SSH_MSG_CHANNEL_REQUEST, connectedList,
        &This::handleChannelRequest);
    setupPacketHandler(SSH_MSG_CHANNEL_OPEN, connectedList,
        &This::handleChannelOpen);
    setupPacketHandler(SSH_MSG_CHANNEL_OPEN_FAILURE, connectedList,
        &This::handleChannelOpenFailure);
    setupPacketHandler(SSH_MSG_CHANNEL_OPEN_CONFIRMATION, connectedList,
        &This::handleChannelOpenConfirmation);
    setupPacketHandler(SSH_MSG_CHANNEL_SUCCESS, connectedList,
        &This::handleChannelSuccess);
    setupPacketHandler(SSH_MSG_CHANNEL_FAILURE, connectedList,
        &This::handleChannelFailure);
    setupPacketHandler(SSH_MSG_CHANNEL_WINDOW_ADJUST, connectedList,
        &This::handleChannelWindowAdjust);
    setupPacketHandler(SSH_MSG_CHANNEL_DATA, connectedList,
        &This::handleChannelData);
    setupPacketHandler(SSH_MSG_CHANNEL_EXTENDED_DATA, connectedList,
        &This::handleChannelExtendedData);

    const StateList connectedOrClosedList
        = StateList() << SocketUnconnected << ConnectionEstablished;
    setupPacketHandler(SSH_MSG_CHANNEL_EOF, connectedOrClosedList,
        &This::handleChannelEof);
    setupPacketHandler(SSH_MSG_CHANNEL_CLOSE, connectedOrClosedList,
        &This::handleChannelClose);

    setupPacketHandler(SSH_MSG_DISCONNECT, StateList() << SocketConnected << WaitingForAgentKeys
        << UserAuthServiceRequested << UserAuthRequested
        << ConnectionEstablished, &This::handleDisconnect);

    setupPacketHandler(SSH_MSG_UNIMPLEMENTED,
        StateList() << ConnectionEstablished, &This::handleUnimplementedPacket);

    setupPacketHandler(SSH_MSG_REQUEST_SUCCESS, connectedList,
        &This::handleRequestSuccess);
    setupPacketHandler(SSH_MSG_REQUEST_FAILURE, connectedList,
        &This::handleRequestFailure);
}

void SshConnectionPrivate::setupPacketHandler(SshPacketType type,
    const SshConnectionPrivate::StateList &states,
    SshConnectionPrivate::PacketHandler handler)
{
    m_packetHandlers.insert(type, HandlerInStates(states, handler));
}

void SshConnectionPrivate::handleSocketConnected()
{
    m_state = SocketConnected;
    sendData(ClientId);
}

void SshConnectionPrivate::handleIncomingData()
{
    if (m_state == SocketUnconnected)
        return; // For stuff queued in the event loop after we've called closeConnection();

    try {
        if (!canUseSocket())
            return;
        m_incomingData += m_socket->readAll();
        qCDebug(sshLog, "state = %d, remote data size = %d", m_state, m_incomingData.count());
        if (m_serverId.isEmpty())
            handleServerId();
        handlePackets();
    } catch (const SshServerException &e) {
        closeConnection(e.error, SshProtocolError, e.errorStringServer,
            tr("SSH Protocol error: %1").arg(e.errorStringUser));
    } catch (const SshClientException &e) {
        closeConnection(SSH_DISCONNECT_BY_APPLICATION, e.error, "",
            e.errorString);
    } catch (const std::exception &e) {
        closeConnection(SSH_DISCONNECT_BY_APPLICATION, SshInternalError, "",
            tr("Botan library exception: %1").arg(QString::fromLocal8Bit(e.what())));
    }
}

// RFC 4253, 4.2.
void SshConnectionPrivate::handleServerId()
{
    qCDebug(sshLog, "%s: incoming data size = %d, incoming data = '%s'",
        Q_FUNC_INFO, m_incomingData.count(), m_incomingData.data());
    const int newLinePos = m_incomingData.indexOf('\n');
    if (newLinePos == -1)
        return; // Not enough data yet.

    // Lines not starting with "SSH-" are ignored.
    if (!m_incomingData.startsWith("SSH-")) {
        m_incomingData.remove(0, newLinePos + 1);
        m_serverHasSentDataBeforeId = true;
        return;
    }

    if (newLinePos > 255 - 1) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Identification string too long.",
            tr("Server identification string is %1 characters long, but the maximum "
               "allowed length is 255.").arg(newLinePos + 1));
    }

    const bool hasCarriageReturn = m_incomingData.at(newLinePos - 1) == '\r';
    m_serverId = m_incomingData.left(newLinePos);
    if (hasCarriageReturn)
        m_serverId.chop(1);
    m_incomingData.remove(0, newLinePos + 1);

    if (m_serverId.contains('\0')) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Identification string contains illegal NUL character.",
            tr("Server identification string contains illegal NUL character."));
    }

    // "printable US-ASCII characters, with the exception of whitespace characters
    // and the minus sign"
    QString legalString = QLatin1String("[]!\"#$!&'()*+,./0-9:;<=>?@A-Z[\\\\^_`a-z{|}~]+");
    const QRegularExpression versionIdpattern(QString::fromLatin1("SSH-(%1)-%1(?: .+)?.*").arg(legalString));
    if (!versionIdpattern.match(QString::fromLatin1(m_serverId)).hasMatch()) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Identification string is invalid.",
            tr("Server Identification string \"%1\" is invalid.")
                    .arg(QString::fromLatin1(m_serverId)));
    }
    const QString serverProtoVersion = versionIdpattern.match(QString::fromLatin1(m_serverId)).captured(1);
    if (serverProtoVersion != QLatin1String("2.0") && serverProtoVersion != QLatin1String("1.99")) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED,
            "Invalid protocol version.",
            tr("Server protocol version is \"%1\", but needs to be 2.0 or 1.99.")
                    .arg(serverProtoVersion));
    }

    if (serverProtoVersion == QLatin1String("2.0") && !hasCarriageReturn) {
        if (m_connParams.options & SshEnableStrictConformanceChecks) {
            throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
                    "Identification string is invalid.",
                    tr("Server identification string is invalid (missing carriage return)."));
        } else {
            qCWarning(Internal::sshLog, "Server identification string is invalid (missing carriage return).");
        }
    }

    if (serverProtoVersion == QLatin1String("1.99") && m_serverHasSentDataBeforeId) {
        if (m_connParams.options & SshEnableStrictConformanceChecks) {
            throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
                    "No extra data preceding identification string allowed for 1.99.",
                    tr("Server reports protocol version 1.99, but sends data "
                        "before the identification string, which is not allowed."));
        } else {
            qCWarning(Internal::sshLog, "Server reports protocol version 1.99, but sends data "
                        "before the identification string, which is not allowed.");
        }
    }

    m_keyExchange.reset(new SshKeyExchange(m_connParams, m_sendFacility));
    m_keyExchange->sendKexInitPacket(m_serverId);
    m_keyExchangeState = KexInitSent;
}

void SshConnectionPrivate::handlePackets()
{
    m_incomingPacket.consumeData(m_incomingData);
    while (m_incomingPacket.isComplete()) {
        handleCurrentPacket();
        m_incomingPacket.clear();
        m_incomingPacket.consumeData(m_incomingData);
    }
}

void SshConnectionPrivate::handleCurrentPacket()
{
    Q_ASSERT(m_incomingPacket.isComplete());
    Q_ASSERT(m_keyExchangeState == DhInitSent || !m_ignoreNextPacket);

    if (m_ignoreNextPacket) {
        m_ignoreNextPacket = false;
        return;
    }

    QHash<SshPacketType, HandlerInStates>::ConstIterator it
        = m_packetHandlers.constFind(m_incomingPacket.type());
    if (it == m_packetHandlers.constEnd()) {
        m_sendFacility.sendMsgUnimplementedPacket(m_incomingPacket.serverSeqNr());
        return;
    }
    if (!it.value().first.contains(m_state)) {
        handleUnexpectedPacket();
        return;
    }
    (this->*it.value().second)();
}

void SshConnectionPrivate::handleKeyExchangeInitPacket()
{
    if (m_keyExchangeState != NoKeyExchange
            && m_keyExchangeState != KexInitSent) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected packet.", tr("Unexpected packet of type %1.")
            .arg(m_incomingPacket.type()));
    }

    // Server-initiated re-exchange.
    if (m_keyExchangeState == NoKeyExchange) {
        m_keyExchange.reset(new SshKeyExchange(m_connParams, m_sendFacility));
        m_keyExchange->sendKexInitPacket(m_serverId);
    }

    // If the server sends a guessed packet, the guess must be wrong,
    // because the algorithms we support require us to initiate the
    // key exchange.
    if (m_keyExchange->sendDhInitPacket(m_incomingPacket)) {
        m_ignoreNextPacket = true;
    }

    m_keyExchangeState = DhInitSent;
}

void SshConnectionPrivate::handleKeyExchangeReplyPacket()
{
    if (m_keyExchangeState != DhInitSent) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected packet.", tr("Unexpected packet of type %1.")
            .arg(m_incomingPacket.type()));
    }

    m_keyExchange->sendNewKeysPacket(m_incomingPacket,
        ClientId.left(ClientId.size() - 2));
    m_hostFingerprint = m_keyExchange->hostKeyFingerprint();
    m_sendFacility.recreateKeys(*m_keyExchange);
    m_keyExchangeState = NewKeysSent;
}

void SshConnectionPrivate::handleNewKeysPacket()
{
    if (m_keyExchangeState != NewKeysSent) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected packet.", tr("Unexpected packet of type %1.")
            .arg(m_incomingPacket.type()));
    }

    m_incomingPacket.recreateKeys(*m_keyExchange);
    m_keyExchange.reset();
    m_keyExchangeState = NoKeyExchange;

    if (m_state == SocketConnected) {
        m_sendFacility.sendUserAuthServiceRequestPacket();
        m_state = UserAuthServiceRequested;
    }
}

void SshConnectionPrivate::handleServiceAcceptPacket()
{
    switch (m_connParams.authenticationType) {
    case SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods:
        m_triedAllPasswordBasedMethods = false;
        Q_FALLTHROUGH();
    case SshConnectionParameters::AuthenticationTypePassword:
        m_sendFacility.sendUserAuthByPasswordRequestPacket(m_connParams.userName().toUtf8(),
                SshCapabilities::SshConnectionService, m_connParams.password().toUtf8());
        break;
    case SshConnectionParameters::AuthenticationTypeKeyboardInteractive:
        m_sendFacility.sendUserAuthByKeyboardInteractiveRequestPacket(m_connParams.userName().toUtf8(),
                SshCapabilities::SshConnectionService);
        break;
    case SshConnectionParameters::AuthenticationTypePublicKey:
        authenticateWithPublicKey();
        break;
    case SshConnectionParameters::AuthenticationTypeAgent:
        if (SshAgent::publicKeys().isEmpty()) {
            if (m_agentKeysUpToDate)
                throw SshClientException(SshAuthenticationError, tr("ssh-agent has no keys."));
            qCDebug(sshLog) << "agent has no keys yet, waiting";
            m_state = WaitingForAgentKeys;
            return;
        } else {
            tryAllAgentKeys();
        }
        break;
    }
    m_state = UserAuthRequested;
}

void SshConnectionPrivate::handlePasswordExpiredPacket()
{
    if (m_connParams.authenticationType == SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
            && m_triedAllPasswordBasedMethods) {
        // This means we just tried to authorize via "keyboard-interactive", in which case
        // this type of packet is not allowed.
        handleUnexpectedPacket();
        return;
    }
    throw SshClientException(SshAuthenticationError, tr("Password expired."));
}

void SshConnectionPrivate::handleUserAuthInfoRequestPacket()
{
    if (m_connParams.authenticationType == SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
            && !m_triedAllPasswordBasedMethods) {
        // This means we just tried to authorize via "password", in which case
        // this type of packet is not allowed.
        handleUnexpectedPacket();
        return;
    }

    const SshUserAuthInfoRequestPacket requestPacket
            = m_incomingPacket.extractUserAuthInfoRequest();
    QStringList responses;
    responses.reserve(requestPacket.prompts.count());

    // Not very interactive, admittedly, but we don't want to be for now.
    for (int i = 0;  i < requestPacket.prompts.count(); ++i)
        responses << m_connParams.password();
    m_sendFacility.sendUserAuthInfoResponsePacket(responses);
}

void SshConnectionPrivate::handleUserAuthBannerPacket()
{
    emit dataAvailable(m_incomingPacket.extractUserAuthBanner().message);
}

void SshConnectionPrivate::handleUnexpectedPacket()
{
    throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
                             "Unexpected packet.", tr("Unexpected packet of type %1.")
                             .arg(m_incomingPacket.type()));
}

void SshConnectionPrivate::handleGlobalRequest()
{
    m_sendFacility.sendRequestFailurePacket();
}

void SshConnectionPrivate::handleUserAuthSuccessPacket()
{
    m_state = ConnectionEstablished;
    m_timeoutTimer.stop();
    emit connected();
    m_lastInvalidMsgSeqNr = InvalidSeqNr;
    connect(&m_keepAliveTimer, &QTimer::timeout, this, &SshConnectionPrivate::sendKeepAlivePacket);
    m_keepAliveTimer.start();
}

void SshConnectionPrivate::handleUserAuthFailurePacket()
{
    if (!m_pendingKeyChecks.isEmpty()) {
        const QByteArray key = m_pendingKeyChecks.dequeue();
        SshAgent::removeDataToSign(key, tokenForAgent());
        qCDebug(sshLog) << "server rejected one of the keys supplied by the agent,"
                        << m_pendingKeyChecks.count() << "keys remaining";
        if (m_pendingKeyChecks.isEmpty() && m_agentKeyToUse.isEmpty()) {
            throw SshClientException(SshAuthenticationError, tr("The server rejected all keys "
                                                                "known to the ssh-agent."));
        }
        return;
    }

    // TODO: Evaluate "authentications that can continue" field and act on it.
    if (m_connParams.authenticationType
            == SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
        && !m_triedAllPasswordBasedMethods) {
        m_triedAllPasswordBasedMethods = true;
        m_sendFacility.sendUserAuthByKeyboardInteractiveRequestPacket(
                    m_connParams.userName().toUtf8(),
                    SshCapabilities::SshConnectionService);
        return;
    }

    m_timeoutTimer.stop();
    QString errorMsg;
    switch (m_connParams.authenticationType) {
    case SshConnectionParameters::AuthenticationTypePublicKey:
    case SshConnectionParameters::AuthenticationTypeAgent:
        errorMsg = tr("Server rejected key.");
        break;
    default:
        errorMsg = tr("Server rejected password.");
        break;
    }
    throw SshClientException(SshAuthenticationError, errorMsg);
}

void SshConnectionPrivate::handleUserAuthKeyOkPacket()
{
    const SshUserAuthPkOkPacket &msg = m_incomingPacket.extractUserAuthPkOk();
    qCDebug(sshLog) << "server accepted key of type" << msg.algoName;

    if (m_pendingKeyChecks.isEmpty()) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR, "Unexpected packet",
                                 tr("Server sent unexpected SSH_MSG_USERAUTH_PK_OK packet."));
    }
    const QByteArray key = m_pendingKeyChecks.dequeue();
    if (key != msg.keyBlob) {
        // The server must answer the requests in the order we sent them.
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR, "Unexpected packet content",
                tr("Server sent unexpected key in SSH_MSG_USERAUTH_PK_OK packet."));
    }
    const uint token = tokenForAgent();
    if (!m_agentKeyToUse.isEmpty()) {
        qCDebug(sshLog) << "another key has already been accepted, ignoring this one";
        SshAgent::removeDataToSign(key, token);
        return;
    }
    m_agentKeyToUse = key;
    qCDebug(sshLog) << "requesting signature from agent";
    SshAgent::requestSignature(key, token);
}

void SshConnectionPrivate::handleDebugPacket()
{
    const SshDebug &msg = m_incomingPacket.extractDebug();
    if (msg.display)
        emit dataAvailable(msg.message);
}

void SshConnectionPrivate::handleUnimplementedPacket()
{
    const SshUnimplemented &msg = m_incomingPacket.extractUnimplemented();
    if (msg.invalidMsgSeqNr != m_lastInvalidMsgSeqNr) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected packet", tr("The server sent an unexpected SSH packet "
            "of type SSH_MSG_UNIMPLEMENTED."));
    }
    m_lastInvalidMsgSeqNr = InvalidSeqNr;
    m_timeoutTimer.stop();
    m_keepAliveTimer.start();
}

void SshConnectionPrivate::handleChannelRequest()
{
    m_channelManager->handleChannelRequest(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelOpen()
{
    m_channelManager->handleChannelOpen(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelOpenFailure()
{
   m_channelManager->handleChannelOpenFailure(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelOpenConfirmation()
{
    m_channelManager->handleChannelOpenConfirmation(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelSuccess()
{
    m_channelManager->handleChannelSuccess(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelFailure()
{
    m_channelManager->handleChannelFailure(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelWindowAdjust()
{
   m_channelManager->handleChannelWindowAdjust(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelData()
{
   m_channelManager->handleChannelData(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelExtendedData()
{
   m_channelManager->handleChannelExtendedData(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelEof()
{
   m_channelManager->handleChannelEof(m_incomingPacket);
}

void SshConnectionPrivate::handleChannelClose()
{
   m_channelManager->handleChannelClose(m_incomingPacket);
}

void SshConnectionPrivate::handleDisconnect()
{
    const SshDisconnect msg = m_incomingPacket.extractDisconnect();
    throw SshServerException(SSH_DISCONNECT_CONNECTION_LOST,
        "", tr("Server closed connection: %1").arg(msg.description));
}

void SshConnectionPrivate::handleRequestSuccess()
{
    m_channelManager->handleRequestSuccess(m_incomingPacket);
}

void SshConnectionPrivate::handleRequestFailure()
{
    m_channelManager->handleRequestFailure(m_incomingPacket);
}

void SshConnectionPrivate::sendData(const QByteArray &data)
{
    if (canUseSocket())
        m_socket->write(data);
}

uint SshConnectionPrivate::tokenForAgent() const
{
    return qHash(m_sendFacility.sessionId());
}

void SshConnectionPrivate::handleSocketDisconnected()
{
    closeConnection(SSH_DISCONNECT_CONNECTION_LOST, SshClosedByServerError,
        "Connection closed unexpectedly.",
        tr("Connection closed unexpectedly."));
}

void SshConnectionPrivate::handleSocketError()
{
    if (m_error == SshNoError) {
        closeConnection(SSH_DISCONNECT_CONNECTION_LOST, SshSocketError,
            "Network error", m_socket->errorString());
    }
}

void SshConnectionPrivate::handleTimeout()
{
    const QString errorMessage = m_state == WaitingForAgentKeys
            ? tr("Timeout waiting for keys from ssh-agent.")
            : tr("Timeout waiting for reply from server.");
    closeConnection(SSH_DISCONNECT_BY_APPLICATION, SshTimeoutError, "", errorMessage);
}

void SshConnectionPrivate::sendKeepAlivePacket()
{
    // This type of message is not allowed during key exchange.
    if (m_keyExchangeState != NoKeyExchange) {
        m_keepAliveTimer.start();
        return;
    }

    Q_ASSERT(m_lastInvalidMsgSeqNr == InvalidSeqNr);
    m_lastInvalidMsgSeqNr = m_sendFacility.nextClientSeqNr();
    m_sendFacility.sendInvalidPacket();
    m_timeoutTimer.start();
}

void SshConnectionPrivate::handleAgentKeysUpdated()
{
    m_agentKeysUpToDate = true;
    if (m_state == WaitingForAgentKeys) {
        m_state = UserAuthRequested;
        tryAllAgentKeys();
    }
}

void SshConnectionPrivate::handleSignatureFromAgent(const QByteArray &key,
                                                    const QByteArray &signature, uint token)
{
    if (token != tokenForAgent()) {
        qCDebug(sshLog) << "signature is for different connection, ignoring";
        return;
    }
    QSSH_ASSERT(key == m_agentKeyToUse);
    m_agentSignature = signature;
    authenticateWithPublicKey();
}

void SshConnectionPrivate::tryAllAgentKeys()
{
    const QList<QByteArray> &keys = SshAgent::publicKeys();
    if (keys.isEmpty())
        throw SshClientException(SshAuthenticationError, tr("ssh-agent has no keys."));
    qCDebug(sshLog) << "trying authentication with" << keys.count()
                    << "public keys received from agent";
    foreach (const QByteArray &key, keys) {
        m_sendFacility.sendQueryPublicKeyPacket(m_connParams.userName().toUtf8(),
                                                SshCapabilities::SshConnectionService, key);
        m_pendingKeyChecks.enqueue(key);
    }
}

void SshConnectionPrivate::authenticateWithPublicKey()
{
    qCDebug(sshLog) << "sending actual authentication request";

    QByteArray key;
    QByteArray signature;
    if (m_connParams.authenticationType == SshConnectionParameters::AuthenticationTypeAgent) {
        // Agent is not needed anymore after this point.
        disconnect(&SshAgent::instance(), nullptr, this, nullptr);

        key = m_agentKeyToUse;
        signature = m_agentSignature;
    }

    m_sendFacility.sendUserAuthByPublicKeyRequestPacket(m_connParams.userName().toUtf8(),
            SshCapabilities::SshConnectionService, key, signature);
}

void SshConnectionPrivate::setAgentError()
{
    m_error = SshAgentError;
    m_errorString = SshAgent::errorString();
    emit error(m_error);
}

void SshConnectionPrivate::connectToHost()
{
    QSSH_ASSERT_AND_RETURN(m_state == SocketUnconnected);

    m_incomingData.clear();
    m_incomingPacket.reset();
    m_sendFacility.reset();
    m_error = SshNoError;
    m_ignoreNextPacket = false;
    m_errorString.clear();
    m_serverId.clear();
    m_serverHasSentDataBeforeId = false;
    m_agentSignature.clear();
    m_agentKeysUpToDate = false;
    m_pendingKeyChecks.clear();
    m_agentKeyToUse.clear();

    switch (m_connParams.authenticationType) {
    case SshConnectionParameters::AuthenticationTypePublicKey:
        try {
            createPrivateKey();
            break;
        } catch (const SshClientException &ex) {
            m_error = ex.error;
            m_errorString = ex.errorString;
            emit error(m_error);
            return;
        }
    case SshConnectionParameters::AuthenticationTypeAgent:
        if (SshAgent::hasError()) {
            setAgentError();
            return;
        }
        connect(&SshAgent::instance(), &SshAgent::errorOccurred,
                this, &SshConnectionPrivate::setAgentError);
        connect(&SshAgent::instance(), &SshAgent::keysUpdated,
                this, &SshConnectionPrivate::handleAgentKeysUpdated);
        SshAgent::refreshKeys();
        connect(&SshAgent::instance(), &SshAgent::signatureAvailable,
                this, &SshConnectionPrivate::handleSignatureFromAgent);
        break;
    default:
        break;
    }

    connect(m_socket, &QAbstractSocket::connected,
            this, &SshConnectionPrivate::handleSocketConnected);
    connect(m_socket, &QIODevice::readyRead,
            this, &SshConnectionPrivate::handleIncomingData);
    //connect(m_socket, &QAbstractSocket::errorOccurred,
    //        this, &SshConnectionPrivate::handleSocketError);
    connect(m_socket, &QAbstractSocket::disconnected,
            this, &SshConnectionPrivate::handleSocketDisconnected);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &SshConnectionPrivate::handleTimeout);
    m_state = SocketConnecting;
    m_keyExchangeState = NoKeyExchange;
    m_timeoutTimer.start();
    m_socket->connectToHost(m_connParams.host(), m_connParams.port());
}

void SshConnectionPrivate::closeConnection(SshErrorCode sshError,
    SshError userError, const QByteArray &serverErrorString,
    const QString &userErrorString)
{
    // Prevent endless loops by recursive exceptions.
    if (m_state == SocketUnconnected || m_error != SshNoError)
        return;

    m_error = userError;
    m_errorString = userErrorString;
    m_timeoutTimer.stop();
    disconnect(m_socket, nullptr, this, nullptr);
    disconnect(&m_timeoutTimer, nullptr, this, nullptr);
    m_keepAliveTimer.stop();
    disconnect(&m_keepAliveTimer, nullptr, this, nullptr);
    try {
        m_channelManager->closeAllChannels(SshChannelManager::CloseAllAndReset);

        // Crypto initialization failed
        if (m_sendFacility.encrypterIsValid()) {
            m_sendFacility.sendDisconnectPacket(sshError, serverErrorString);
        }
    } catch (...) {}  // Nothing sensible to be done here.
    if (m_error != SshNoError)
        emit error(userError);
    if (m_state == ConnectionEstablished)
        emit disconnected();
    if (canUseSocket())
        m_socket->disconnectFromHost();
    m_state = SocketUnconnected;
}

bool SshConnectionPrivate::canUseSocket() const
{
    return m_socket->isValid()
            && m_socket->state() == QAbstractSocket::ConnectedState;
}

void SshConnectionPrivate::createPrivateKey()
{
    if (m_connParams.privateKeyFile.isEmpty())
        throw SshClientException(SshKeyFileError, tr("No private key file given."));
    QFile keyFile(m_connParams.privateKeyFile);
//    if (!keyFile.open(QIODevice::ReadOnly)) {
//        throw SshClientException(SshKeyFileError,
//            tr("Private key file error: %1").arg(keyFile.errorString()));
//    }
//    m_sendFacility.createAuthenticationKey(keyFile.readAll());

    // Patch supporting storing key in pass field
    if (keyFile.open(QIODevice::ReadOnly)) {
        m_sendFacility.createAuthenticationKey(keyFile.readAll());
    }
    else {
        m_sendFacility.createAuthenticationKey(m_connParams.privateKeyFile.toUtf8());
    }
}

QSharedPointer<SshRemoteProcess> SshConnectionPrivate::createRemoteProcess(const QByteArray &command)
{
    return m_channelManager->createRemoteProcess(command);
}

QSharedPointer<SshRemoteProcess> SshConnectionPrivate::createRemoteShell()
{
    return m_channelManager->createRemoteShell();
}

QSharedPointer<SftpChannel> SshConnectionPrivate::createSftpChannel()
{
    return m_channelManager->createSftpChannel();
}

SshDirectTcpIpTunnel::Ptr SshConnectionPrivate::createDirectTunnel(const QString &originatingHost,
        quint16 originatingPort, const QString &remoteHost, quint16 remotePort)
{
    return m_channelManager->createDirectTunnel(originatingHost, originatingPort, remoteHost,
                                                remotePort);
}

SshTcpIpForwardServer::Ptr SshConnectionPrivate::createForwardServer(const QString &bindAddress,
        quint16 bindPort)
{
    return m_channelManager->createForwardServer(bindAddress, bindPort);
}

const quint64 SshConnectionPrivate::InvalidSeqNr = static_cast<quint64>(-1);

} // namespace Internal
} // namespace QSsh
