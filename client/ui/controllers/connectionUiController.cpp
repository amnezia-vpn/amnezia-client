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
        m_awgStateTimer.stop();
        amnApp->networkManager()->clearConnectionCache();

        m_isConnectionInProgress = false;
        m_isConnected = true;
        m_connectionStateText = tr("Connected");
        break;
    }
    case Vpn::ConnectionState::Connecting: {
        {
            const QString serverId = m_serversController->getDefaultServerId();
            if (!serverId.isEmpty()) {
                const DockerContainer container = m_serversController->getDefaultContainer(serverId);
                const Proto proto = ContainerUtils::defaultProtocol(container);
                if (proto == Proto::Awg) {
                    m_awgStateTimer.start(10000);
                } else {
                    m_awgStateTimer.stop();
                }
            }
        }
        m_isConnectionInProgress = true;
        break;
    }
    case Vpn::ConnectionState::Reconnecting: {
        m_awgStateTimer.stop();
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Reconnecting...");
        break;
    }
    case Vpn::ConnectionState::Disconnected: {
        m_awgStateTimer.stop();
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        break;
    }
    case Vpn::ConnectionState::Disconnecting: {
        m_awgStateTimer.stop();
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Disconnecting...");
        break;
    }
    case Vpn::ConnectionState::Preparing: {
        m_awgStateTimer.stop();
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Preparing...");
        break;
    }
    case Vpn::ConnectionState::Error: {
        m_awgStateTimer.stop();
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    case Vpn::ConnectionState::Unknown: {
        m_awgStateTimer.stop();
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

    const QMap<DockerContainer, ContainerConfig> containersMap = m_serversController->getServerContainersMap(serverId);
    if (!containersMap.contains(DockerContainer::Xray)) {
        qDebug().noquote() << "AWG connect timeout: no Xray container available";
        return;
    }

    qDebug().noquote() << "AWG connect timeout (10s), switching default container to Xray and reconnecting";

    m_serversController->setDefaultContainer(serverId, DockerContainer::Xray);
    emit requestSetCurrentProtocol(QStringLiteral("vless"));

    closeConnection();

    QTimer::singleShot(500, this, [this]() {
        if (!m_isConnected && !m_isConnectionInProgress) {
            emit prepareConfig();
        }
    });
}
