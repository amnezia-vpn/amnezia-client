#include "connectionController.h"

#include <QJsonDocument>

#include "core/configurators/configuratorBase.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/utilities.h"
#include "core/utils/networkUtilities.h"
#include "version.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

ConnectionController::ConnectionController(SecureServersRepository* serversRepository,
                                         SecureAppSettingsRepository* appSettingsRepository,
                                         VpnConnection* vpnConnection,
                                         QObject* parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_vpnConnection(vpnConnection)
{
    connect(m_vpnConnection, &VpnConnection::connectionStateChanged, this, &ConnectionController::connectionStateChanged);
}

bool ConnectionController::isConnected() const
{
    return m_vpnConnection && m_vpnConnection->connectionState() == Vpn::ConnectionState::Connected;
}

void ConnectionController::setConnectionState(Vpn::ConnectionState state)
{
    if (m_vpnConnection) {
        m_vpnConnection->setConnectionState(state);
    }
}

ErrorCode ConnectionController::prepareConnection(int serverIndex,
                                                 QJsonObject& vpnConfiguration,
                                                 DockerContainer& container)
{
    if (!isServiceReady()) {
        return ErrorCode::AmneziaServiceNotRunning;
    }

    ServerConfig serverConfigModel = m_serversRepository->server(serverIndex);
    container = serverConfigModel.defaultContainer();

    if (!isContainerSupported(container)) {
        return ErrorCode::NotSupportedOnThisPlatform;
    }

    ContainerConfig containerConfigModel = m_serversRepository->containerConfig(serverIndex, container);

    auto dns = serverConfigModel.getDnsPair(m_appSettingsRepository->useAmneziaDns(),
                                            m_appSettingsRepository->primaryDns(),
                                            m_appSettingsRepository->secondaryDns());

    vpnConfiguration = createConnectionConfiguration(dns, serverConfigModel, containerConfigModel, container);

    return ErrorCode::NoError;
}

ErrorCode ConnectionController::connectToVpn(int serverIndex)
{
    QJsonObject vpnConfiguration;
    DockerContainer container;

    ErrorCode errorCode = prepareConnection(serverIndex, vpnConfiguration, container);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    m_vpnConnection->connectToVpn(serverIndex, container, vpnConfiguration);
    return ErrorCode::NoError;
}

void ConnectionController::disconnectFromVpn()
{
    m_vpnConnection->disconnectFromVpn();
}

#ifdef Q_OS_ANDROID
void ConnectionController::restoreConnection()
{
    if (m_vpnConnection) {
        m_vpnConnection->restoreConnection();
    }
}
#endif

void ConnectionController::onKillSwitchModeChanged(bool enabled)
{
    if (m_vpnConnection) {
        m_vpnConnection->onKillSwitchModeChanged(enabled);
    }
}

ErrorCode ConnectionController::lastError() const
{
    return m_vpnConnection->lastError();
}

QJsonObject ConnectionController::createConnectionConfiguration(const QPair<QString, QString> &dns,
                                                              const ServerConfig &serverConfig,
                                                              const ContainerConfig &containerConfig,
                                                              DockerContainer container)
{
    QJsonObject vpnConfiguration {};

    if (ContainerUtils::containerService(container) == ServiceType::Other) {
        return vpnConfiguration;
    }

    Proto proto = ContainerUtils::defaultProtocol(container);

    ConnectionSettings connectionSettings = {
        { dns.first, dns.second },
        serverConfig.isApiConfig(),
        {
            m_appSettingsRepository->isSitesSplitTunnelingEnabled(),
            m_appSettingsRepository->routeMode()
        }
    };

    auto configurator = ConfiguratorBase::create(proto, nullptr);
    ProtocolConfig processedConfig = configurator->processConfigWithLocalSettings(connectionSettings,
                                                                                  containerConfig.protocolConfig);

    QJsonObject vpnConfigData = processedConfig.getClientConfigJson();
    if (ContainerUtils::isAwgContainer(container) || container == DockerContainer::WireGuard) {
        if (vpnConfigData[configKey::mtu].toString().isEmpty()) {
            vpnConfigData[configKey::mtu] =
                    ContainerUtils::isAwgContainer(container) ? protocols::awg::defaultMtu :
                    protocols::wireguard::defaultMtu;
        }
    }

    vpnConfiguration.insert(ProtocolUtils::key_proto_config_data(proto), vpnConfigData);
    vpnConfiguration[configKey::vpnProto] = ProtocolUtils::protoToString(proto);

    vpnConfiguration[configKey::dns1] = dns.first;
    vpnConfiguration[configKey::dns2] = dns.second;

    vpnConfiguration[configKey::hostName] = serverConfig.hostName();
    vpnConfiguration[configKey::description] = serverConfig.description();

    vpnConfiguration[configKey::configVersion] = serverConfig.configVersion();

    return vpnConfiguration;
}

bool ConnectionController::isServiceReady() const
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    return Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true);
#else
    return true;
#endif
}

bool ConnectionController::isContainerSupported(DockerContainer container) const
{
    return ContainerUtils::isSupportedByCurrentPlatform(container);
}
