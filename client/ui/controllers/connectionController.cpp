#include "connectionController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif
#include <QTimer>

#include "amnezia_application.h"
#include "core/api/apiUtils.h"
#include "logger.h"
#include "utilities.h"
#include "core/controllers/vpnConfigurationController.h"
#include "version.h"

namespace
{
    Logger logger("ConnectionController");
}

ConnectionController::ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ContainersModel> &containersModel,
                                           const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                           const QSharedPointer<VpnConnection> &vpnConnection, const std::shared_ptr<Settings> &settings,
                                           QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_vpnConnection(vpnConnection),
      m_settings(settings)
{
    logger.debug() << "Created. Initial state disconnected";
    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, this, &ConnectionController::onConnectionStateChanged);
    connect(this, &ConnectionController::connectToVpn, m_vpnConnection.get(), &VpnConnection::connectToVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::disconnectFromVpn, m_vpnConnection.get(), &VpnConnection::disconnectFromVpn, Qt::QueuedConnection);

    connect(this, &ConnectionController::connectButtonClicked, this, &ConnectionController::toggleConnection, Qt::QueuedConnection);

    m_state = Vpn::ConnectionState::Disconnected;
}

void ConnectionController::openConnection()
{
    logger.debug() << "openConnection requested. State:" << m_state
                   << "isConnected:" << m_isConnected
                   << "inProgress:" << m_isConnectionInProgress;
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    if (!Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true))
    {
        logger.error() << "Service is not running, cannot open connection";
        emit connectionErrorOccurred(ErrorCode::AmneziaServiceNotRunning);
        return;
    }
#endif

    int serverIndex = m_serversModel->getDefaultServerIndex();
    logger.debug() << "Default server index:" << static_cast<uint64_t>(serverIndex);
    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIndex);

    DockerContainer container = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));
    logger.debug() << "Default container:" << container;

    if (!m_containersModel->isSupportedByCurrentPlatform(container)) {
        logger.error() << "Container is not supported on current platform:" << container;
        emit connectionErrorOccurred(ErrorCode::NotSupportedOnThisPlatform);
        return;
    }

    QSharedPointer<ServerController> serverController(new ServerController(m_settings));
    VpnConfigurationsController vpnConfigurationController(m_settings, serverController);

    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);

    auto dns = m_serversModel->getDnsPair(serverIndex);

    auto vpnConfiguration = vpnConfigurationController.createVpnConfiguration(dns, serverConfig, containerConfig, container);
    logger.debug() << "VPN configuration prepared, emitting connectToVpn";
    emit connectToVpn(serverIndex, credentials, container, vpnConfiguration);
}

void ConnectionController::closeConnection()
{
    logger.debug() << "closeConnection requested. State:" << m_state
                   << "isConnected:" << m_isConnected
                   << "inProgress:" << m_isConnectionInProgress;
    emit disconnectFromVpn();
}

ErrorCode ConnectionController::getLastConnectionError()
{
    return m_vpnConnection->lastError();
}

void ConnectionController::onConnectionStateChanged(Vpn::ConnectionState state)
{
    logger.debug() << "Connection state changed from" << m_state << "to" << state
                   << "lastError:" << getLastConnectionError();
    m_state = state;

    m_isConnected = false;
    m_connectionStateText = tr("Connecting...");
    switch (state) {
    case Vpn::ConnectionState::Connected: {
        amnApp->networkManager()->clearConnectionCache();

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

void ConnectionController::suppressNextQueuedToggle(int timeoutMs)
{
    m_suppressNextQueuedToggle = true;
    const quint64 generation = ++m_toggleSuppressionGeneration;
    logger.debug() << "Suppressing next queued toggleConnection request for" << static_cast<uint64_t>(timeoutMs)
                   << "ms. generation:" << generation;

    QTimer::singleShot(timeoutMs, this, [this, generation]() {
        if (!m_suppressNextQueuedToggle || m_toggleSuppressionGeneration != generation) {
            return;
        }

        logger.debug() << "Queued toggle suppression expired. generation:" << generation;
        m_suppressNextQueuedToggle = false;
    });
}

void ConnectionController::toggleConnection()
{
    QObject *const source = sender();
    logger.debug() << "toggleConnection sender:"
                   << (source ? source->metaObject()->className() : "nullptr")
                   << "suppressed:" << m_suppressNextQueuedToggle;

    if (m_suppressNextQueuedToggle && source == this && !isConnected() && !isConnectionInProgress()) {
        logger.warning() << "Suppressing queued toggleConnection request after external disconnect";
        m_suppressNextQueuedToggle = false;
        return;
    }

    logger.debug() << "toggleConnection called. State:" << m_state
                   << "isConnected:" << isConnected()
                   << "inProgress:" << isConnectionInProgress();
    if (m_state == Vpn::ConnectionState::Preparing) {
        logger.debug() << "Already preparing, emitting preparingConfig";
        emit preparingConfig();
        return;
    }

    if (isConnectionInProgress()) {
        logger.debug() << "Connection in progress, closing connection";
        closeConnection();
    } else if (isConnected()) {
        logger.debug() << "Already connected, closing connection";
        closeConnection();
    } else {
        logger.debug() << "Disconnected, emitting prepareConfig";
        emit prepareConfig();
    }
}

void ConnectionController::toggleConnectionByShortcut()
{
    logger.debug() << "toggleConnectionByShortcut called. State:" << m_state
                   << "isConnected:" << isConnected()
                   << "inProgress:" << isConnectionInProgress()
                   << "sender:" << (sender() ? sender()->metaObject()->className() : "nullptr");
    if (m_state == Vpn::ConnectionState::Preparing) {
        logger.debug() << "Shortcut hit during preparing, emitting preparingConfig";
        emit preparingConfig();
        return;
    }

    if (isConnectionInProgress() || isConnected()) {
        logger.debug() << "Shortcut requests disconnect";
        suppressNextQueuedToggle();
        closeConnection();
        return;
    }

    // Skip validation when cached protocol configs are already available.
    if (hasReadyConnectionConfig()) {
        logger.debug() << "Shortcut found ready config, opening connection directly";
        openConnection();
        return;
    }

    logger.debug() << "Shortcut requires prepareConfig before connect";
    emit prepareConfig();
}

bool ConnectionController::isConnectionInProgress() const
{
    return m_isConnectionInProgress;
}

bool ConnectionController::isConnected() const
{
    return m_isConnected;
}

bool ConnectionController::hasReadyConnectionConfig() const
{
    const int serverIndex = m_serversModel->getDefaultServerIndex();
    if (serverIndex < 0) {
        logger.debug() << "hasReadyConnectionConfig: no default server";
        return false;
    }

    const QJsonObject serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    if (apiUtils::isServerFromApi(serverConfigObject)) {
        logger.debug() << "hasReadyConnectionConfig: server comes from API, config considered ready";
        return true;
    }

    if (!m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        logger.debug() << "hasReadyConnectionConfig: server has no installed containers";
        return false;
    }

    const auto container = qvariant_cast<DockerContainer>(m_serversModel->data(serverIndex, ServersModel::Roles::DefaultContainerRole));
    if (container == DockerContainer::None) {
        logger.debug() << "hasReadyConnectionConfig: default container is None";
        return false;
    }

    const QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    for (const Proto protocol : ContainerProps::protocolsForContainer(container)) {
        const QString protocolConfig =
                containerConfig.value(ProtocolProps::protoToString(protocol)).toObject().value(config_key::last_config).toString();
        if (protocolConfig.isEmpty()) {
            logger.debug() << "hasReadyConnectionConfig: missing cached config for protocol" << protocol;
            return false;
        }
    }

    logger.debug() << "hasReadyConnectionConfig: all cached configs found";
    return true;
}
