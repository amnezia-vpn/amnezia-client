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

ErrorCode ConnectionController::prepareConnection(int serverIndex,
                                                 QJsonObject& vpnConfiguration,
                                                 ServerCredentials& credentials,
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
    credentials = m_serversRepository->serverCredentials(serverIndex);

    auto dns = serverConfigModel.getDnsPair(m_appSettingsRepository->useAmneziaDns(),
                                            m_appSettingsRepository->primaryDns(),
                                            m_appSettingsRepository->secondaryDns());

    vpnConfiguration = createConnectionConfiguration(dns, serverConfigModel, containerConfigModel, container);

    return ErrorCode::NoError;
}

ErrorCode ConnectionController::connectToVpn(int serverIndex)
{
    QJsonObject vpnConfiguration;
    ServerCredentials credentials;
    DockerContainer container;

    ErrorCode errorCode = prepareConnection(serverIndex, vpnConfiguration, credentials, container);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    m_vpnConnection->connectToVpn(serverIndex, credentials, container, vpnConfiguration);
    return ErrorCode::NoError;
}

void ConnectionController::disconnectFromVpn()
{
    m_vpnConnection->disconnectFromVpn();
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

    bool isApiConfig = serverConfig.isApiConfig();
    Proto proto = ContainerUtils::defaultProtocol(container);

    QJsonObject protocolConfigJson = containerConfig.protocolConfig.toJson();
    QString protocolConfigString = protocolConfigJson.value(config_key::last_config).toString();

    SplitTunnelingSettings splitTunneling = {
        m_appSettingsRepository->isSitesSplitTunnelingEnabled(),
        m_appSettingsRepository->routeMode()
    };

    auto configurator = ConfiguratorBase::create(proto, nullptr);
    protocolConfigString = configurator->processConfigWithLocalSettings(dns, isApiConfig, splitTunneling, protocolConfigString);

    QJsonObject vpnConfigData = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();
    if (ContainerUtils::isAwgContainer(container) || container == DockerContainer::WireGuard) {
        if (vpnConfigData[config_key::mtu].toString().isEmpty()) {
            vpnConfigData[config_key::mtu] =
                    ContainerUtils::isAwgContainer(container) ? protocols::awg::defaultMtu :
                    protocols::wireguard::defaultMtu;
        }
    }

    vpnConfiguration.insert(ProtocolUtils::key_proto_config_data(proto), vpnConfigData);
    vpnConfiguration[config_key::vpnproto] = ProtocolUtils::protoToString(proto);

    vpnConfiguration[config_key::dns1] = dns.first;
    vpnConfiguration[config_key::dns2] = dns.second;

    vpnConfiguration[config_key::hostName] = serverConfig.hostName();
    vpnConfiguration[config_key::description] = serverConfig.description();

    vpnConfiguration[config_key::configVersion] = serverConfig.configVersion();

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
