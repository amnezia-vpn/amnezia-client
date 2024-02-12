#include "connectionController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "core/errorstrings.h"

ConnectionController::ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ContainersModel> &containersModel,
                                           const QSharedPointer<VpnConnection> &vpnConnection, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel), m_vpnConnection(vpnConnection)
{
    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, this,
            &ConnectionController::onConnectionStateChanged);
    connect(m_vpnConnection.get(), &VpnConnection::bytesChanged, this, [this](quint64 rx, quint64 tx)
            {
                m_rxBytes = rx;
                m_txBytes = tx;
            });
    connect(this, &ConnectionController::connectToVpn, m_vpnConnection.get(), &VpnConnection::connectToVpn,
            Qt::QueuedConnection);
    connect(this, &ConnectionController::disconnectFromVpn, m_vpnConnection.get(), &VpnConnection::disconnectFromVpn,
            Qt::QueuedConnection);
    connect(&m_tick, &QTimer::timeout, this, [this]()
            {
                quint64 time = QDateTime::currentSecsSinceEpoch();
                if (m_times.length() > viewSize)
                {
                    m_times.removeFirst();
                    m_rxView.removeFirst();
                    m_txView.removeFirst();
                }
                m_times.append(time);
                m_rxView.append(m_rxBytes);
                m_txView.append(m_txBytes);
                emit bytesChanged();
            });

    m_state = Vpn::ConnectionState::Disconnected;
}

void ConnectionController::openConnection()
{
    if (!m_containersModel->isAnyContainerInstalled()) {
        emit noInstalledContainers();
        return;
    }

    int serverIndex = m_serversModel->getDefaultServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    DockerContainer container = m_containersModel->getDefaultContainer();
    const QJsonObject &containerConfig = m_containersModel->getContainerConfig(container);

    if (container == DockerContainer::None) {
        emit connectionErrorOccurred(tr("VPN Protocols is not installed.\n Please install VPN container at first"));
        return;
    }

    qApp->processEvents();

    emit connectToVpn(serverIndex, credentials, container, containerConfig);
}

void ConnectionController::closeConnection()
{
    emit disconnectFromVpn();
}

QString ConnectionController::getLastConnectionError()
{
    return errorString(m_vpnConnection->lastError());
}

void ConnectionController::onConnectionStateChanged(Vpn::ConnectionState state)
{
    m_state = state;

    m_isConnected = false;
    m_connectionStateText = tr("Connection...");
    switch (state) {
    case Vpn::ConnectionState::Connected: {
        m_isConnectionInProgress = false;
        m_isConnected = true;
        m_connectionStateText = tr("Connected");
        m_tick.start(1000);
        break;
    }
    case Vpn::ConnectionState::Connecting: {
        m_isConnectionInProgress = true;
        break;
    }
    case Vpn::ConnectionState::Reconnecting: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Reconnection...");
        break;
    }
    case Vpn::ConnectionState::Disconnected: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        break;
    }
    case Vpn::ConnectionState::Disconnecting: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Disconnection...");
        m_tick.stop();
        break;
    }
    case Vpn::ConnectionState::Preparing: {
        m_isConnectionInProgress = true;
        break;
    }
    case Vpn::ConnectionState::Error: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    case Vpn::ConnectionState::Unknown: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        emit connectionErrorOccurred(getLastConnectionError());
        break;
    }
    }
    emit connectionStateChanged();
}

void ConnectionController::onCurrentContainerUpdated()
{
    if (m_isConnected || m_isConnectionInProgress) {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully, Reconnnection..."));
        openConnection();
    } else {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully"));
    }
}

void ConnectionController::onTranslationsUpdated()
{
    // get translated text of current state
    onConnectionStateChanged(getCurrentConnectionState());
}

Vpn::ConnectionState ConnectionController::getCurrentConnectionState()
{
    return m_state;
}

QString ConnectionController::connectionStateText() const
{
    return m_connectionStateText;
}

quint64 ConnectionController::rxBytes() const
{
    return m_rxBytes;
}

quint64 ConnectionController::txBytes() const
{
    return m_txBytes;
}

QVector<quint64> ConnectionController::getRxView() const
{
    return m_rxView;
}

QVector<quint64> ConnectionController::getTxView() const
{
    return m_txView;
}

QVector<quint64> ConnectionController::getTimes() const
{
    return m_times;
}

void ConnectionController::toggleConnection(bool skipConnectionInProgressCheck)
{
    if (!skipConnectionInProgressCheck && isConnectionInProgress()) {
        closeConnection();
    } else if (isConnected()) {
        closeConnection();
    } else {
        openConnection();
    }
}

bool ConnectionController::isConnectionInProgress() const
{
    return m_isConnectionInProgress;
}

bool ConnectionController::isConnected() const
{
    return m_isConnected;
}
