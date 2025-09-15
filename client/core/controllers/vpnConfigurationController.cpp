#include "vpnConfigurationController.h"

#include "configurators/awg_configurator.h"
#include "configurators/cloak_configurator.h"
#include "configurators/ikev2_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "configurators/xray_configurator.h"

#include "core/models/protocols/torProtocolConfig.h"

VpnConfigurationsController::VpnConfigurationsController(const std::shared_ptr<Settings> &settings,
                                                         QSharedPointer<ServerController> serverController, QObject *parent)
    : QObject { parent }, m_settings(settings), m_serverController(serverController)
{
}

QScopedPointer<ConfiguratorBase> VpnConfigurationsController::createConfigurator(const Proto protocol)
{
    switch (protocol) {
    case Proto::OpenVpn: return QScopedPointer<ConfiguratorBase>(new OpenVpnConfigurator(m_settings, m_serverController));
    case Proto::ShadowSocks: return QScopedPointer<ConfiguratorBase>(new ShadowSocksConfigurator(m_settings, m_serverController));
    case Proto::Cloak: return QScopedPointer<ConfiguratorBase>(new CloakConfigurator(m_settings, m_serverController));
    case Proto::WireGuard: return QScopedPointer<ConfiguratorBase>(new WireguardConfigurator(m_settings, m_serverController, false));
    case Proto::Awg: return QScopedPointer<ConfiguratorBase>(new AwgConfigurator(m_settings, m_serverController));
    case Proto::Ikev2: return QScopedPointer<ConfiguratorBase>(new Ikev2Configurator(m_settings, m_serverController));
    case Proto::Xray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(m_settings, m_serverController));
    case Proto::SSXray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(m_settings, m_serverController));
    default: return QScopedPointer<ConfiguratorBase>();
    }
}

ErrorCode VpnConfigurationsController::createClientProtocolConfigs(const ServerCredentials &serverCredentials,
                                                                   ContainerConfig &containerConfig)
{
    ErrorCode errorCode = ErrorCode::NoError;

    if (ContainerProps::containerService(containerConfig.containerType) == ServiceType::Other) {
        return errorCode;
    }

    for (Proto protocol : ContainerProps::protocolsForContainer(containerConfig.containerType)) {
        auto configurator = createConfigurator(protocol);
        auto protocolConfig = configurator->createConfig(serverCredentials, containerConfig, errorCode);
        if (errorCode != ErrorCode::NoError) {
            return errorCode;
        }

        containerConfig.protocolConfigs[protocolConfig->protocolName] = protocolConfig;
    }

    return errorCode;
}

ErrorCode VpnConfigurationsController::createClientProtocolConfig(const ServerCredentials &serverCredentials,
                                                                  const ContainerConfig &containerConfig, const Proto protocol,
                                                                  QSharedPointer<ProtocolConfig> &protocolConfig)
{
    ErrorCode errorCode = ErrorCode::NoError;

    if (ContainerProps::containerService(containerConfig.containerType) == ServiceType::Other) {
        return errorCode;
    }

    auto configurator = createConfigurator(protocol);
    protocolConfig = configurator->createConfig(serverCredentials, containerConfig, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    return errorCode;
}

void VpnConfigurationsController::processNativeConfigForExport(const QPair<QString, QString> &dns,
                                                               QSharedPointer<ProtocolConfig> &protocolConfig)
{
    auto configurator = createConfigurator(protocolConfig->protocolType);
    protocolConfig = configurator->processConfigWithExportSettings(dns, protocolConfig);
}

QJsonObject VpnConfigurationsController::createVpnConfiguration(const QPair<QString, QString> &dns, const ContainerConfig &containerConfig,
                                                                const DockerContainer containerType, const int configVersion,
                                                                const QString &hostName)
{
    QJsonObject vpnConfiguration {};

    if (ContainerProps::containerService(containerType) == ServiceType::Other) {
        return vpnConfiguration;
    }

    bool isApiConfig = configVersion;

    for (ProtocolEnumNS::Proto proto : ContainerProps::protocolsForContainer(containerType)) {
        if (isApiConfig && containerType == DockerContainer::Cloak && proto == ProtocolEnumNS::Proto::ShadowSocks) {
            continue;
        }

        QString protocolName = ProtocolProps::protoToString(proto);
        auto protocolConfig = containerConfig.protocolConfigs.value(protocolName);

        if (protocolConfig) {
            auto configurator = createConfigurator(proto);
            protocolConfig = configurator->processConfigWithLocalSettings(dns, isApiConfig, protocolConfig);

            QJsonObject protocolJson = protocolConfig->toJson();
            QString protocolConfigString = protocolJson.value(config_key::last_config).toString();
            QJsonObject vpnConfigData = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();

            if (containerType == DockerContainer::Awg || containerType == DockerContainer::WireGuard) {
                // add mtu for old configs
                if (vpnConfigData[config_key::mtu].toString().isEmpty()) {
                    vpnConfigData[config_key::mtu] =
                            containerType == DockerContainer::Awg ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;
                }
            }

            vpnConfiguration.insert(ProtocolProps::key_proto_config_data(proto), vpnConfigData);
        }
    }

    Proto proto = ContainerProps::defaultProtocol(containerType);
    vpnConfiguration[config_key::vpnproto] = ProtocolProps::protoToString(proto);

    vpnConfiguration[config_key::dns1] = dns.first;
    vpnConfiguration[config_key::dns2] = dns.second;

    vpnConfiguration[config_key::hostName] = hostName;
    vpnConfiguration[config_key::description] = hostName; // TODO: might need description field in ServerConfig

    vpnConfiguration[config_key::configVersion] = configVersion;
    // TODO: try to get hostName, port, description for 3rd party configs
    // vpnConfiguration[config_key::port] = ...;

    return vpnConfiguration;
}

void VpnConfigurationsController::updateContainerConfigAfterInstallation(const DockerContainer container, ContainerConfig &containerConfig,
                                                                         const QString &stdOut)
{
    if (container == DockerContainer::TorWebSite) {
        Proto mainProto = ContainerProps::defaultProtocol(container);
        QString protocolName = ProtocolProps::protoToString(mainProto);
        auto protocolConfig = containerConfig.protocolConfigs.value(protocolName);

        if (protocolConfig) {
            qDebug() << "amnezia-tor onions" << stdOut;

            QString onion = stdOut;
            onion.replace("\n", "");

            auto torConfig = qSharedPointerCast<TorProtocolConfig>(protocolConfig);
            if (torConfig) {
                torConfig->serverProtocolConfig.site = onion;
            }
        }
    }
}
