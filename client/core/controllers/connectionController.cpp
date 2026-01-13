#include "connectionController.h"

#include <QJsonDocument>

#include "core/configurators/configuratorBase.h"
#include "protocols/protocols_defs.h"
#include "core/utils/utilities.h"
#include "core/utils/networkUtilities.h"
#include "version.h"
#include "containers/containers_defs.h"

ConnectionController::ConnectionController(ServersRepository* serversRepository,
                                         AppSettingsRepository* appSettingsRepository,
                                         VpnConnection* vpnConnection,
                                         const std::shared_ptr<Settings> &settings)
    : m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_vpnConnection(vpnConnection),
      m_settings(settings)
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

    QJsonObject serverConfig = m_serversRepository->server(serverIndex);

    QString defaultContainerString = serverConfig.value(config_key::defaultContainer).toString();
    container = ContainerProps::containerFromString(defaultContainerString);

    if (!isContainerSupported(container)) {
        return ErrorCode::NotSupportedOnThisPlatform;
    }

    QJsonObject containerConfig = m_serversRepository->containerConfig(serverIndex, container);

    credentials = m_serversRepository->serverCredentials(serverIndex);

    auto dns = getDnsPair(serverIndex, m_appSettingsRepository->useAmneziaDns());

    vpnConfiguration = createConnectionConfiguration(dns, serverConfig, containerConfig, container);

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

QPair<QString, QString> ConnectionController::getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const
{
    QPair<QString, QString> dns;

    const QJsonObject &server = m_serversRepository->server(serverIndex);
    const auto containers = server.value(config_key::containers).toArray();

    bool isDnsContainerInstalled = false;
    for (const QJsonValue &container : containers) {
        if (ContainerProps::containerFromString(container.toObject().value(config_key::container).toString()) == DockerContainer::Dns) {
            isDnsContainerInstalled = true;
        }
    }

    dns.first = server.value(config_key::dns1).toString();
    dns.second = server.value(config_key::dns2).toString();

    if (dns.first.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.first)) {
        if (isAmneziaDnsEnabled && isDnsContainerInstalled) {
            dns.first = protocols::dns::amneziaDnsIp;
        } else {
            dns.first = m_appSettingsRepository->primaryDns();
        }
    }

    if (dns.second.isEmpty() || !NetworkUtilities::checkIPv4Format(dns.second)) {
        dns.second = m_appSettingsRepository->secondaryDns();
    }

    return dns;
}

QJsonObject ConnectionController::createConnectionConfiguration(const QPair<QString, QString> &dns,
                                                              const QJsonObject &serverConfig,
                                                              const QJsonObject &containerConfig,
                                                              DockerContainer container)
{
    QJsonObject vpnConfiguration {};

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return vpnConfiguration;
    }

    bool isApiConfig = serverConfig.value(config_key::configVersion).toInt();

    for (Proto proto : ContainerProps::protocolsForContainer(container)) {
        if (isApiConfig && container == DockerContainer::Cloak && proto == Proto::ShadowSocks) {
            continue;
        }

        QString protocolConfigString =
                containerConfig.value(ProtocolProps::protoToString(proto)).toObject().value(config_key::last_config).toString();

        auto configurator = ConfiguratorBase::create(proto, m_settings, nullptr);
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
    }

    Proto proto = ContainerProps::defaultProtocol(container);
    vpnConfiguration[config_key::vpnproto] = ProtocolProps::protoToString(proto);

    vpnConfiguration[config_key::dns1] = dns.first;
    vpnConfiguration[config_key::dns2] = dns.second;

    vpnConfiguration[config_key::hostName] = serverConfig.value(config_key::hostName).toString();
    vpnConfiguration[config_key::description] = serverConfig.value(config_key::description).toString();

    vpnConfiguration[config_key::configVersion] = serverConfig.value(config_key::configVersion).toInt();

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
