#include "connectionUIController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "core/controllers/vpnConfigurationController.h"
#include "core/models/containers/containers_defs.h"
#include "version.h"

ConnectionUIController::ConnectionUIController(const QSharedPointer<ServersModel> &serversModel,
                                               const QSharedPointer<ContainersModel> &containersModel,
                                               const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                               const QSharedPointer<ConnectionController> &connectionController, const std::shared_ptr<Settings> &settings,
                                               QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_connectionController(connectionController),
      m_settings(settings)
{
    connect(m_connectionController.get(), &ConnectionController::connectionEstablished, this, [this]() {
        onConnectionStateChanged(Vpn::ConnectionState::Connected);
    });
    connect(m_connectionController.get(), &ConnectionController::connectionTerminated, this, [this]() {
        onConnectionStateChanged(Vpn::ConnectionState::Disconnected);
    });
    connect(m_connectionController.get(), &ConnectionController::connectionError, this, [this](ErrorCode) {
        onConnectionStateChanged(Vpn::ConnectionState::Error);
    });

    connect(this, &ConnectionUIController::connectButtonClicked, this, &ConnectionUIController::toggleConnection, Qt::QueuedConnection);

    m_state = Vpn::ConnectionState::Disconnected;
}

void ConnectionUIController::openConnection()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    auto serverConfig = m_serversModel->getServerConfig(serverIndex);

    DockerContainer container = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    auto dns = m_serversModel->getDnsPair(serverIndex);

    m_connectionController->openConnection(serverIndex, serverConfig, container, credentials, dns);
}

void ConnectionUIController::closeConnection()
{
    m_connectionController->closeConnection();
}

ErrorCode ConnectionUIController::getLastConnectionError()
{
    return m_connectionController->getLastConnectionError();
}

void ConnectionUIController::onConnectionStateChanged(Vpn::ConnectionState state)
{
    m_state = state;

    m_isConnected = false;
    m_connectionStateText = tr("Connecting...");
    switch (state) {
    case Vpn::ConnectionState::Connected: {
        m_isConnectionInProgress = false;
        m_isConnected = true;
        m_connectionStateText = tr("Connected");
        break;
    }
    case Vpn::ConnectionState::Connecting: {
        m_isConnectionInProgress = true;
        break;
    }
    case Vpn::ConnectionState::Reconnecting: {
        m_isConnectionInProgress = true;
        m_connectionStateText = tr("Reconnecting...");
        break;
    }
    case Vpn::ConnectionState::Disconnected: {
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
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

void ConnectionUIController::onCurrentContainerUpdated()
{
    if (m_isConnected || m_isConnectionInProgress) {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully, reconnnection..."));
        openConnection();
    } else {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully"));
    }
}

void ConnectionUIController::onTranslationsUpdated()
{
    // get translated text of current state
    onConnectionStateChanged(getCurrentConnectionState());
}

Vpn::ConnectionState ConnectionUIController::getCurrentConnectionState()
{
    return m_state;
}

QString ConnectionUIController::connectionStateText() const
{
    return m_connectionStateText;
}

void ConnectionUIController::toggleConnection()
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

bool ConnectionUIController::isConnectionInProgress() const
{
    return m_isConnectionInProgress;
}

bool ConnectionUIController::isConnected() const
{
    return m_isConnected;
}
