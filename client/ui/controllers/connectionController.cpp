#include "connectionController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "core/controllers/vpnConfigurationController.h"
#include "version.h"

ConnectionController::ConnectionController(ServersController* serversController,
                                          ServersModel* serversModel,
                                          ContainersModel* containersModel,
                                          ClientManagementModel* clientManagementModel,
                                          VpnConnection* vpnConnection,
                                          QAppSettingsRepository* appSettingsRepository,
                                          const std::shared_ptr<Settings> &settings,
                                          QObject *parent)
    : QObject(parent),
      m_serversController(serversController),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_vpnConnection(vpnConnection),
      m_appSettingsRepository(appSettingsRepository),
      m_settings(settings)
{
    connect(m_vpnConnection, &VpnConnection::connectionStateChanged, this, &ConnectionController::onConnectionStateChanged);
    connect(this, &ConnectionController::connectToVpn, m_vpnConnection, &VpnConnection::connectToVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::disconnectFromVpn, m_vpnConnection, &VpnConnection::disconnectFromVpn, Qt::QueuedConnection);

    connect(this, &ConnectionController::connectButtonClicked, this, &ConnectionController::toggleConnection, Qt::QueuedConnection);

    m_state = Vpn::ConnectionState::Disconnected;
}

void ConnectionController::openConnection()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    if (!Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true))
    {
        emit connectionErrorOccurred(ErrorCode::AmneziaServiceNotRunning);
        return;
    }
#endif

    int serverIndex = m_serversController->getDefaultServerIndex();
    QJsonObject serverConfig = m_serversController->getServerConfig(serverIndex);

    DockerContainer container = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));

    if (!m_containersModel->isSupportedByCurrentPlatform(container)) {
        emit connectionErrorOccurred(ErrorCode::NotSupportedOnThisPlatform);
        return;
    }

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    ServerCredentials credentials = m_serversController->getServerCredentials(serverIndex);

    auto dns = m_serversController->getDnsPair(serverIndex, m_appSettingsRepository->useAmneziaDns());

    auto vpnConfiguration = vpnConfigurationController.createVpnConfiguration(dns, serverConfig, containerConfig, container);
    emit connectToVpn(serverIndex, credentials, container, vpnConfiguration);
}

void ConnectionController::closeConnection()
{
    emit disconnectFromVpn();
}

ErrorCode ConnectionController::getLastConnectionError()
{
    return m_vpnConnection->lastError();
}

void ConnectionController::onConnectionStateChanged(Vpn::ConnectionState state)
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

void ConnectionController::onCurrentContainerUpdated()
{
    if (m_isConnected || m_isConnectionInProgress) {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully, reconnnection..."));
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

void ConnectionController::toggleConnection()
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

bool ConnectionController::isConnectionInProgress() const
{
    return m_isConnectionInProgress;
}

bool ConnectionController::isConnected() const
{
    return m_isConnected;
}
