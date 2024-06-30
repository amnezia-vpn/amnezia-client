#include "connectionController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif
#include <QtConcurrent>

#include "core/controllers/vpnConfigurationController.h"
#include "core/errorstrings.h"
#include "version.h"

ConnectionController::ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ContainersModel> &containersModel,
                                           const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                           const QSharedPointer<VpnConnection> &vpnConnection, const std::shared_ptr<Settings> &settings,
                                           QObject *parent)
    : QObject(parent),
      m_apiController(this),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_vpnConnection(vpnConnection),
      m_settings(settings)
{
    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, this, &ConnectionController::onConnectionStateChanged);
    connect(this, &ConnectionController::connectToVpn, m_vpnConnection.get(), &VpnConnection::connectToVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::disconnectFromVpn, m_vpnConnection.get(), &VpnConnection::disconnectFromVpn, Qt::QueuedConnection);

    connect(&m_apiController, &ApiController::configUpdated, this,
            static_cast<void (ConnectionController::*)(const bool, const QJsonObject &, const int)>(&ConnectionController::openConnection));
    connect(&m_apiController, &ApiController::errorOccurred, this, &ConnectionController::connectionErrorOccurred);

    m_state = Vpn::ConnectionState::Disconnected;
}

void ConnectionController::openConnection()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (!Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true))
    {
        emit connectionErrorOccurred(errorString(ErrorCode::AmneziaServiceNotRunning));
        return;
    }
#endif

    int serverIndex = m_serversModel->getDefaultServerIndex();
    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIndex);

    emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Preparing);

    if (serverConfig.value(config_key::configVersion).toInt()
        && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        m_apiController.updateServerConfigFromApi(m_settings->getInstallationUuid(true), serverIndex, serverConfig);
    } else {
        openConnection(false, serverConfig, serverIndex);
    }
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

void ConnectionController::waitForConnectionFinished(int msecs)
{
    m_vpnConnection->waitForVpnConnectionFinished(msecs);
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

bool ConnectionController::isProtocolConfigExists(const QJsonObject &containerConfig, const DockerContainer container)
{
    for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
        QString protocolConfig =
                containerConfig.value(ProtocolProps::protoToString(protocol)).toObject().value(config_key::last_config).toString();

        if (protocolConfig.isEmpty()) {
            return false;
        }
    }
    return true;
}

void ConnectionController::openConnection(const bool updateConfig, const QJsonObject &config, const int serverIndex)
{
    // Update config for this server as it was received from API
    if (updateConfig) {
        m_serversModel->editServer(config, serverIndex);
    }

    if (!m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        emit noInstalledContainers();
        emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
        return;
    }

    DockerContainer container = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));

    if (!m_containersModel->isSupportedByCurrentPlatform(container)) {
        emit connectionErrorOccurred(tr("The selected protocol is not supported on the current platform"));
        return;
    }

    if (container == DockerContainer::None) {
        emit connectionErrorOccurred(tr("VPN Protocols is not installed.\n Please install VPN container at first"));
        return;
    }

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    ErrorCode errorCode = updateProtocolConfig(container, credentials, containerConfig, serverController);
    if (errorCode != ErrorCode::NoError) {
        emit connectionErrorOccurred(errorString(errorCode));
        return;
    }

    auto dns = m_serversModel->getDnsPair(serverIndex);

    auto vpnConfiguration = vpnConfigurationController.createVpnConfiguration(dns, config, containerConfig, container, errorCode);
    if (errorCode != ErrorCode::NoError) {
        emit connectionErrorOccurred(tr("unable to create configuration"));
        return;
    }

    emit connectToVpn(serverIndex, credentials, container, vpnConfiguration);
}

ErrorCode ConnectionController::updateProtocolConfig(const DockerContainer container, const ServerCredentials &credentials,
                                                     QJsonObject &containerConfig, QSharedPointer<ServerController> serverController)
{
    QFutureWatcher<ErrorCode> watcher;

    if (serverController.isNull()) {
        serverController.reset(new ServerController(m_settings));
    }

    QFuture<ErrorCode> future = QtConcurrent::run([this, container, &credentials, &containerConfig, &serverController]() {
        ErrorCode errorCode = ErrorCode::NoError;
        if (!isProtocolConfigExists(containerConfig, container)) {
            VpnConfigurationsController vpnConfigurationController(m_settings, serverController);
            errorCode = vpnConfigurationController.createProtocolConfigForContainer(credentials, container, containerConfig);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
            m_serversModel->updateContainerConfig(container, containerConfig);

            errorCode = m_clientManagementModel->appendClient(container, credentials, containerConfig,
                                                              QString("Admin [%1]").arg(QSysInfo::prettyProductName()), serverController);
            if (errorCode != ErrorCode::NoError) {
                return errorCode;
            }
        }
        return errorCode;
    });

    QEventLoop wait;
    connect(&watcher, &QFutureWatcher<ErrorCode>::finished, &wait, &QEventLoop::quit);
    watcher.setFuture(future);
    wait.exec();

    return watcher.result();
}
