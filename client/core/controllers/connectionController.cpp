#include "connectionController.h"

#include <QJsonDocument>

#include "core/configurators/configuratorBase.h"
#include "protocols/protocols_defs.h"
#include "core/utils/utilities.h"
#include "core/utils/networkUtilities.h"
#include "version.h"
#include "containers/containers_defs.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

ConnectionController::ConnectionController(QServersRepository* serversRepository,
                                         QAppSettingsRepository* appSettingsRepository,
                                         VpnConnection* vpnConnection)
    : m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_vpnConnection(vpnConnection)
{
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
    container = ServerConfigUtils::defaultContainer(serverConfigModel);

    if (!isContainerSupported(container)) {
        return ErrorCode::NotSupportedOnThisPlatform;
    }

    ContainerConfig containerConfigModel = m_serversRepository->containerConfig(serverIndex, container);
    credentials = m_serversRepository->serverCredentials(serverIndex);

    auto dns = ServerConfigUtils::getDnsPair(serverConfigModel, 
                                              m_appSettingsRepository->useAmneziaDns(),
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

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return vpnConfiguration;
    }

    bool isApiConfig = ServerConfigUtils::isApiConfig(serverConfig);
    Proto proto = ContainerProps::defaultProtocol(container);

    QJsonObject protocolConfigJson = ProtocolConfigUtils::toJson(containerConfig.protocolConfig, proto);
    QString protocolConfigString = protocolConfigJson.value(config_key::last_config).toString();

    auto configurator = ConfiguratorBase::create(proto, m_appSettingsRepository, nullptr);
    protocolConfigString = configurator->processConfigWithLocalSettings(dns, isApiConfig, protocolConfigString);

    QJsonObject vpnConfigData = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();
    if (ContainerProps::isAwgContainer(container) || container == DockerContainer::WireGuard) {
        if (vpnConfigData[config_key::mtu].toString().isEmpty()) {
            vpnConfigData[config_key::mtu] =
                    ContainerProps::isAwgContainer(container) ? protocols::awg::defaultMtu :
                    protocols::wireguard::defaultMtu;
        }
    }

    vpnConfiguration.insert(ProtocolProps::key_proto_config_data(proto), vpnConfigData);
    vpnConfiguration[config_key::vpnproto] = ProtocolProps::protoToString(proto);

    vpnConfiguration[config_key::dns1] = dns.first;
    vpnConfiguration[config_key::dns2] = dns.second;

    vpnConfiguration[config_key::hostName] = ServerConfigUtils::hostName(serverConfig);
    vpnConfiguration[config_key::description] = ServerConfigUtils::description(serverConfig);

    vpnConfiguration[config_key::configVersion] = ServerConfigUtils::configVersion(serverConfig);

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
    return ContainerProps::isSupportedByCurrentPlatform(container);
}
