#include "connectionUiController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "amneziaApplication.h"
#include "core/controllers/serversController.h"
#include "core/models/containerConfig.h"
#include "core/utils/containerEnum.h"

ConnectionUiController::ConnectionUiController(ConnectionController* connectionController,
                                                ServersController* serversController,
                                                QObject *parent)
    : QObject(parent),
      m_connectionController(connectionController),
      m_serversController(serversController)
{
    connect(m_connectionController, &ConnectionController::connectionStateChanged, this, &ConnectionUiController::onConnectionStateChanged);
    connect(m_connectionController, &ConnectionController::bytesChanged, this, &ConnectionUiController::onBytesChanged);

    connect(this, &ConnectionUiController::connectButtonClicked, this, &ConnectionUiController::toggleConnection, Qt::QueuedConnection);

    m_statsTimer.setInterval(1000);
    connect(&m_statsTimer, &QTimer::timeout, this, &ConnectionUiController::connectionStatsChanged);

    m_state = Vpn::ConnectionState::Disconnected;
}

void ConnectionUiController::openConnection()
{
    const QString serverId = m_serversController->getDefaultServerId();
    if (serverId.isEmpty()) {
        m_connectionController->setConnectionState(Vpn::ConnectionState::Disconnected);
        return;
    }

    ErrorCode errorCode = m_connectionController->openConnection(serverId);

    if (errorCode != ErrorCode::NoError) {
        notifyConnectionBlocked(errorCode);
        return;
    }
}

void ConnectionUiController::closeConnection()
{
    m_connectionController->closeConnection();
}

ErrorCode ConnectionUiController::getLastConnectionError()
{
    return m_connectionController->lastConnectionError();
}

void ConnectionUiController::onConnectionStateChanged(Vpn::ConnectionState state)
{
    const Vpn::ConnectionState previousState = m_state;
    m_state = state;

    m_isConnected = false;
    m_connectionStateText = tr("Connecting...");
    switch (state) {
    case Vpn::ConnectionState::Connected: {
        amnApp->networkManager()->clearConnectionCache();

        m_isConnectionInProgress = false;
        m_isConnected = true;
        m_connectionStateText = tr("Connected");

        if (m_attemptInProgress && !m_setupCaptured) {
            m_connectionSetupMs = static_cast<int>(m_connectionAttemptTimer.elapsed());
            m_setupCaptured = true;
            m_attemptInProgress = false;
        }
        if (!m_connectedTimer.isValid()) {
            m_connectedTimer.start();
        }
        m_speedTimer.start();
        m_statsTimer.start();
        break;
    }
    case Vpn::ConnectionState::Connecting: {
        m_isConnectionInProgress = true;
        if (!m_attemptInProgress) {
            m_attemptInProgress = true;
            m_setupCaptured = false;
            m_connectionSetupMs = 0;
            m_receivedBytes = 0;
            m_sentBytes = 0;
            m_downloadSpeed = 0;
            m_uploadSpeed = 0;
            m_connectionAttemptTimer.start();
        }
        break;
    }
    case Vpn::ConnectionState::Reconnecting: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Reconnecting...");
        if (!m_attemptInProgress) {
            m_attemptInProgress = true;
            m_setupCaptured = false;
            m_connectionSetupMs = 0;
            m_receivedBytes = 0;
            m_sentBytes = 0;
            m_downloadSpeed = 0;
            m_uploadSpeed = 0;
            m_connectionAttemptTimer.start();
        }
        break;
    }
    case Vpn::ConnectionState::Disconnected: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        m_attemptInProgress = false;
        m_statsTimer.stop();
        m_speedTimer.invalidate();
        m_downloadSpeed = 0;
        m_uploadSpeed = 0;
        m_connectedTimer.invalidate();
        break;
    }
    case Vpn::ConnectionState::Disconnecting: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Disconnecting...");
        break;
    }
    case Vpn::ConnectionState::Preparing: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Preparing...");
        if (previousState == Vpn::ConnectionState::Disconnected
                || previousState == Vpn::ConnectionState::Error
                || previousState == Vpn::ConnectionState::Unknown) {
            m_attemptInProgress = true;
            m_setupCaptured = false;
            m_connectionSetupMs = 0;
            m_receivedBytes = 0;
            m_sentBytes = 0;
            m_downloadSpeed = 0;
            m_uploadSpeed = 0;
            m_connectionAttemptTimer.start();
        }
        break;
    }
    case Vpn::ConnectionState::Error: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        m_attemptInProgress = false;
        m_statsTimer.stop();
        m_speedTimer.invalidate();
        m_downloadSpeed = 0;
        m_uploadSpeed = 0;
        m_connectedTimer.invalidate();
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    case Vpn::ConnectionState::Unknown: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        m_attemptInProgress = false;
        m_statsTimer.stop();
        m_speedTimer.invalidate();
        m_downloadSpeed = 0;
        m_uploadSpeed = 0;
        m_connectedTimer.invalidate();
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    }
    emit connectionStateChanged();
    emit connectionStatsChanged();
}

void ConnectionUiController::onTranslationsUpdated()
{
    onConnectionStateChanged(getCurrentConnectionState());
}

Vpn::ConnectionState ConnectionUiController::getCurrentConnectionState()
{
    return m_state;
}

QString ConnectionUiController::connectionStateText() const
{
    return m_connectionStateText;
}

void ConnectionUiController::toggleConnection()
{
    if (m_state == Vpn::ConnectionState::Preparing) {
        emit preparingConfig();
        return;
    }

    if (isConnectionInProgress()) {
        closeConnection();
    } else if (isConnected()) {
        closeConnection();
    } else {
        const QString serverId = m_serversController->getDefaultServerId();
        if (serverId.isEmpty()) {
            return;
        }

        const ErrorCode errorCode = m_connectionController->isConnectionSupported(serverId);
        if (errorCode != ErrorCode::NoError) {
            notifyConnectionBlocked(errorCode);
            return;
        }

        emit prepareConfig();
    }
}

void ConnectionUiController::notifyConnectionBlocked(ErrorCode errorCode)
{
    if (errorCode == ErrorCode::LegacyApiV1NotSupportedError) {
        emit unsupportedConnectDrawerRequested();
        return;
    }

    if (errorCode == ErrorCode::NoInstalledContainersError) {
        emit noInstalledContainers();
        return;
    }

    emit connectionErrorOccurred(errorCode);
}

bool ConnectionUiController::isConnectionInProgress() const
{
    return m_isConnectionInProgress;
}

bool ConnectionUiController::isConnected() const
{
    return m_isConnected;
}

int ConnectionUiController::connectionDurationSecs() const
{
    if (!m_isConnected || !m_connectedTimer.isValid()) {
        return 0;
    }
    return static_cast<int>(m_connectedTimer.elapsed() / 1000);
}

int ConnectionUiController::connectionSetupMs() const
{
    return m_connectionSetupMs;
}

qint64 ConnectionUiController::receivedBytes() const
{
    return m_receivedBytes;
}

qint64 ConnectionUiController::sentBytes() const
{
    return m_sentBytes;
}

qint64 ConnectionUiController::downloadSpeed() const
{
    return m_downloadSpeed;
}

qint64 ConnectionUiController::uploadSpeed() const
{
    return m_uploadSpeed;
}

void ConnectionUiController::onBytesChanged(quint64 receivedBytes, quint64 sentBytes)
{
    if (!m_isConnected) {
        return;
    }

    const qint64 elapsedMs = m_speedTimer.isValid() ? m_speedTimer.restart() : 1000;
    const qint64 divisor = elapsedMs > 0 ? elapsedMs : 1000;

    m_downloadSpeed = static_cast<qint64>((receivedBytes * 1000) / divisor);
    m_uploadSpeed = static_cast<qint64>((sentBytes * 1000) / divisor);
    m_receivedBytes += static_cast<qint64>(receivedBytes);
    m_sentBytes += static_cast<qint64>(sentBytes);
    emit connectionStatsChanged();
}

bool ConnectionUiController::isRevokeBlockedDuringActiveConnection(const QString &serverId, int containerIndex,
                                                                   const QString &clientId) const
{
    if (clientId.isEmpty() || (!isConnected() && !isConnectionInProgress())) {
        return false;
    }

    if (m_serversController->getDefaultServerId() != serverId) {
        return false;
    }

    if (static_cast<int>(m_serversController->getDefaultContainer(serverId)) != containerIndex) {
        return false;
    }

    const auto adminConfig = m_serversController->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return false;
    }

    const QString connectionClientId =
            adminConfig->containerConfig(static_cast<DockerContainer>(containerIndex)).protocolConfig.clientId();
    if (connectionClientId.isEmpty()) {
        return false;
    }

    return connectionClientId == clientId || connectionClientId.contains(clientId);
}
