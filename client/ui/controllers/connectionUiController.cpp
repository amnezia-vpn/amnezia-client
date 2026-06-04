#include "connectionUiController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "amneziaApplication.h"
#include "core/controllers/serversController.h"
#include "core/utils/containers/containerUtils.h"

ConnectionUiController::ConnectionUiController(ConnectionController* connectionController,
                                                ServersController* serversController,
                                                QObject *parent)
    : QObject(parent),
      m_connectionController(connectionController),
      m_serversController(serversController)
{
    connect(m_connectionController, &ConnectionController::connectionStateChanged, this, &ConnectionUiController::onConnectionStateChanged);

    connect(this, &ConnectionUiController::connectButtonClicked, this, &ConnectionUiController::toggleConnection, Qt::QueuedConnection);

    m_awgStateTimer.setSingleShot(true);
    connect(&m_awgStateTimer, &QTimer::timeout, this, &ConnectionUiController::onAwgStateTimeout);

    m_state = Vpn::ConnectionState::Disconnected;
}

void ConnectionUiController::openConnection()
{
    const QString serverId = m_serversController->getDefaultServerId();
    if (serverId.isEmpty()) {
        return;
    }

    ErrorCode errorCode = m_connectionController->openConnection(serverId);

    if (errorCode != ErrorCode::NoError) {
        emit connectionErrorOccurred(errorCode);
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
    m_state = state;

    m_isConnected = false;
    m_connectionStateText = tr("Connecting...");
    switch (state) {
    case Vpn::ConnectionState::Connected: {
        if (m_awgStateTimer.isActive()) {
            m_awgStateTimer.stop();
        }
        amnApp->networkManager()->clearConnectionCache();

        m_isConnectionInProgress = false;
        m_isConnected = true;
        m_connectionStateText = tr("Connected");
        break;
    }
    case Vpn::ConnectionState::Connecting: {
        checkAndStartAwgStateTimer();
        m_isConnectionInProgress = true;
        break;
    }
    case Vpn::ConnectionState::Reconnecting: {
        if (m_awgStateTimer.isActive()) {
            m_awgStateTimer.stop();
        }
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Reconnecting...");
        break;
    }
    case Vpn::ConnectionState::Disconnected: {
        if (m_awgStateTimer.isActive()) {
            m_awgStateTimer.stop();
        }
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        break;
    }
    case Vpn::ConnectionState::Disconnecting: {
        if (m_awgStateTimer.isActive()) {
            m_awgStateTimer.stop();
        }
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Disconnecting...");
        break;
    }
    case Vpn::ConnectionState::Preparing: {
        if (m_awgStateTimer.isActive()) {
            m_awgStateTimer.stop();
        }
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Preparing...");
        break;
    }
    case Vpn::ConnectionState::Error: {
        if (m_awgStateTimer.isActive()) {
            m_awgStateTimer.stop();
        }
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    case Vpn::ConnectionState::Unknown: {
        if (m_awgStateTimer.isActive()) {
            m_awgStateTimer.stop();
        }
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    }
    emit connectionStateChanged();
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
        emit prepareConfig();
    }
}

bool ConnectionUiController::isConnectionInProgress() const
{
    return m_isConnectionInProgress;
}

bool ConnectionUiController::isConnected() const
{
    return m_isConnected;
}

void ConnectionUiController::onCurrentContainerUpdated()
{
    if (m_isConnected || m_isConnectionInProgress) {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully, reconnecting..."));
        openConnection();
    } else {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully"));
    }
}

void ConnectionUiController::checkAndStartAwgStateTimer()
{
    const QString serverId = m_serversController->getDefaultServerId();
    if (serverId.isEmpty()) {
        if (m_awgStateTimer.isActive()) {
            m_awgStateTimer.stop();
        }
        return;
    }

    const DockerContainer container = m_serversController->getDefaultContainer(serverId);
    const Proto proto = ContainerUtils::defaultProtocol(container);
    if (proto == Proto::Awg) {
        const auto v2Config = m_serversController->apiV2Config(serverId);
        if (v2Config.has_value() && (v2Config->isPremium() || v2Config->isExternalPremium())) {
            const bool isAutoMode = v2Config->serviceProtocol().isEmpty();
            if (isAutoMode) {
                if (!m_awgStateTimer.isActive()) {
                    m_awgStateTimer.start(kAwgSwitchTimeoutMs);
                }
                return;
            }
        }
        else if (m_serversController->isLegacyApiV1Server(serverId)) {
            if (!m_awgStateTimer.isActive()) {
                m_awgStateTimer.start(kAwgSwitchTimeoutMs);
            }
            return;
        }
    }
    if (m_awgStateTimer.isActive()) {
        m_awgStateTimer.stop();
    }
}

void ConnectionUiController::onAwgStateTimeout()
{
    if (m_state != Vpn::ConnectionState::Connecting) {
        return;
    }

    const QString serverId = m_serversController->getDefaultServerId();
    if (serverId.isEmpty()) {
        return;
    }

    const DockerContainer container = m_serversController->getDefaultContainer(serverId);
    const Proto proto = ContainerUtils::defaultProtocol(container);
    if (proto != Proto::Awg) {
        return;
    }

    closeConnection();

    QTimer::singleShot(1000, this, [this, serverId]() {
        if (m_isConnected || m_isConnectionInProgress) {
            return;
        }

        qDebug().noquote() << "AWG connect timeout: trying to switch API protocol to VLESS and reload config from gateway";

        m_pendingApiServerId = serverId;
        m_apiSwitched = false;
        m_waitingForApiUpdate = true;

        emit requestSetProcessedServer(serverId);
        emit requestSetCurrentProtocol(serverId, QStringLiteral("vless"));
        emit requestUpdateServiceFromGateway(serverId, QString(), QString(), true);
    });
}

void ConnectionUiController::onUpdateServiceFromGatewayCompleted(bool success, const QString &serverId)
{
    if (!m_waitingForApiUpdate || m_pendingApiServerId != serverId) {
        return;
    }

    m_waitingForApiUpdate = false;
    m_apiSwitched = success;

    if (success) {
        const QMap<DockerContainer, ContainerConfig> containersMap = m_serversController->getServerContainersMap(serverId);
        if (containersMap.contains(DockerContainer::Xray)) {
            qDebug().noquote() << "AWG connect timeout (10s), switching default container to Xray and reconnecting";
            m_serversController->setDefaultContainer(serverId, DockerContainer::Xray);
            emit requestSetCurrentProtocol(serverId, QStringLiteral("vless"));
            m_pendingApiServerId.clear();

            if (!m_isConnected && !m_isConnectionInProgress) {
                emit prepareConfig();
            }
            return;
        }
    }

    qDebug().noquote() << "AWG connect timeout: no Xray available (API switch success ="
                       << (m_apiSwitched ? "YES" : "NO") << ")";
    m_pendingApiServerId.clear();
}
