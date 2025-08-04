#include "vpnConfigurationController.h"

#include "configurators/awg_configurator.h"
#include "configurators/cloak_configurator.h"
#include "configurators/ikev2_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "configurators/xray_configurator.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"

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

QSharedPointer<ProtocolConfig> VpnConfigurationsController::createProtocolConfig(const Proto protocol, const QJsonObject &protocolConfigJson)
{
    switch (protocol) {
    case Proto::OpenVpn: 
        return QSharedPointer<OpenVpnProtocolConfig>::create(protocolConfigJson, ProtocolProps::protoToString(protocol));
    case Proto::ShadowSocks: 
        return QSharedPointer<ShadowsocksProtocolConfig>::create(protocolConfigJson, ProtocolProps::protoToString(protocol));
    case Proto::Cloak: 
        return QSharedPointer<CloakProtocolConfig>::create(protocolConfigJson, ProtocolProps::protoToString(protocol));
    case Proto::WireGuard: 
        return QSharedPointer<WireGuardProtocolConfig>::create(protocolConfigJson, ProtocolProps::protoToString(protocol));
    case Proto::Awg: 
        return QSharedPointer<AwgProtocolConfig>::create(protocolConfigJson, ProtocolProps::protoToString(protocol));
    case Proto::Xray: 
    case Proto::SSXray: 
        return QSharedPointer<XrayProtocolConfig>::create(protocolConfigJson, ProtocolProps::protoToString(protocol));
    case Proto::Ikev2: 
        return QSharedPointer<Ikev2ProtocolConfig>::create(ProtocolProps::protoToString(protocol));
    default: 
        return nullptr;
    }
}

ErrorCode VpnConfigurationsController::createProtocolConfigForContainer(const ServerCredentials &credentials,
                                                                        const DockerContainer container, QJsonObject &containerConfig)
{
    ErrorCode errorCode = ErrorCode::NoError;

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return errorCode;
    }

    for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
        QJsonObject protocolConfigJson = containerConfig.value(ProtocolProps::protoToString(protocol)).toObject();

        // Create ProtocolConfig from JSON
        auto protocolConfig = createProtocolConfig(protocol, protocolConfigJson);
        if (!protocolConfig) {
            errorCode = ErrorCode::InternalError;
            return errorCode;
        }

        auto configurator = createConfigurator(protocol);
        auto result = configurator->createConfig(credentials, container, protocolConfig, errorCode);
        if (errorCode != ErrorCode::NoError || !result) {
            return errorCode;
        }

        // Extract nativeConfig and store back in JSON for backward compatibility
        QString nativeConfig;
        if (auto openVpnConfig = qSharedPointerCast<OpenVpnProtocolConfig>(result)) {
            nativeConfig = openVpnConfig->clientProtocolConfig.nativeConfig;
        } else if (auto wgConfig = qSharedPointerCast<WireGuardProtocolConfig>(result)) {
            nativeConfig = wgConfig->clientProtocolConfig.nativeConfig;
        } else if (auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(result)) {
            nativeConfig = awgConfig->clientProtocolConfig.nativeConfig;
        } else if (auto cloakConfig = qSharedPointerCast<CloakProtocolConfig>(result)) {
            nativeConfig = cloakConfig->clientProtocolConfig.nativeConfig;
        } else if (auto xrayConfig = qSharedPointerCast<XrayProtocolConfig>(result)) {
            nativeConfig = xrayConfig->clientProtocolConfig.nativeConfig;
        } else if (auto shadowsocksConfig = qSharedPointerCast<ShadowsocksProtocolConfig>(result)) {
            nativeConfig = shadowsocksConfig->clientProtocolConfig.nativeConfig;
        } else if (auto ikev2Config = qSharedPointerCast<Ikev2ProtocolConfig>(result)) {
            nativeConfig = ikev2Config->clientProtocolConfig.nativeConfig;
        }

        protocolConfigJson.insert(config_key::last_config, nativeConfig);
        containerConfig.insert(ProtocolProps::protoToString(protocol), protocolConfigJson);
    }

    return errorCode;
}

ErrorCode VpnConfigurationsController::createProtocolConfigString(const bool isApiConfig, const QPair<QString, QString> &dns,
                                                                  const ServerCredentials &credentials, const DockerContainer container,
                                                                  const QJsonObject &containerConfig, const Proto protocol,
                                                                  QString &protocolConfigString)
{
    ErrorCode errorCode = ErrorCode::NoError;

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return errorCode;
    }

    // Create ProtocolConfig from JSON
    QJsonObject protocolConfigJson = containerConfig.value(ProtocolProps::protoToString(protocol)).toObject();
    auto protocolConfig = createProtocolConfig(protocol, protocolConfigJson);
    if (!protocolConfig) {
        errorCode = ErrorCode::InternalError;
        return errorCode;
    }

    auto configurator = createConfigurator(protocol);
    auto result = configurator->createConfig(credentials, container, protocolConfig, errorCode);
    if (errorCode != ErrorCode::NoError || !result) {
        return errorCode;
    }

    // Extract nativeConfig
    QString nativeConfig;
    if (auto openVpnConfig = qSharedPointerCast<OpenVpnProtocolConfig>(result)) {
        nativeConfig = openVpnConfig->clientProtocolConfig.nativeConfig;
    } else if (auto wgConfig = qSharedPointerCast<WireGuardProtocolConfig>(result)) {
        nativeConfig = wgConfig->clientProtocolConfig.nativeConfig;
    } else if (auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(result)) {
        nativeConfig = awgConfig->clientProtocolConfig.nativeConfig;
    } else if (auto cloakConfig = qSharedPointerCast<CloakProtocolConfig>(result)) {
        nativeConfig = cloakConfig->clientProtocolConfig.nativeConfig;
    } else if (auto xrayConfig = qSharedPointerCast<XrayProtocolConfig>(result)) {
        nativeConfig = xrayConfig->clientProtocolConfig.nativeConfig;
    } else if (auto shadowsocksConfig = qSharedPointerCast<ShadowsocksProtocolConfig>(result)) {
        nativeConfig = shadowsocksConfig->clientProtocolConfig.nativeConfig;
    } else if (auto ikev2Config = qSharedPointerCast<Ikev2ProtocolConfig>(result)) {
        nativeConfig = ikev2Config->clientProtocolConfig.nativeConfig;
    }

    protocolConfigString = configurator->processConfigWithExportSettings(dns, isApiConfig, nativeConfig);

    return errorCode;
}

QJsonObject VpnConfigurationsController::createVpnConfiguration(const QPair<QString, QString> &dns, const QJsonObject &serverConfig,
                                                                const QJsonObject &containerConfig, const DockerContainer container)
{
    QJsonObject vpnConfiguration {};

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return vpnConfiguration;
    }

    bool isApiConfig = serverConfig.value(config_key::configVersion).toInt();

    for (ProtocolEnumNS::Proto proto : ContainerProps::protocolsForContainer(container)) {
        if (isApiConfig && container == DockerContainer::Cloak && proto == ProtocolEnumNS::Proto::ShadowSocks) {
            continue;
        }

        QString protocolConfigString =
                containerConfig.value(ProtocolProps::protoToString(proto)).toObject().value(config_key::last_config).toString();

        auto configurator = createConfigurator(proto);
        protocolConfigString = configurator->processConfigWithLocalSettings(dns, isApiConfig, protocolConfigString);

        QJsonObject vpnConfigData = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();
        if (container == DockerContainer::Awg || container == DockerContainer::WireGuard) {
            // add mtu for old configs
            if (vpnConfigData[config_key::mtu].toString().isEmpty()) {
                vpnConfigData[config_key::mtu] =
                        container == DockerContainer::Awg ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;
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
    // TODO: try to get hostName, port, description for 3rd party configs
    // vpnConfiguration[config_key::port] = ...;

    return vpnConfiguration;
}

void VpnConfigurationsController::updateContainerConfigAfterInstallation(const DockerContainer container, QJsonObject &containerConfig,
                                                                         const QString &stdOut)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    if (container == DockerContainer::TorWebSite) {
        QJsonObject protocol = containerConfig.value(ProtocolProps::protoToString(mainProto)).toObject();

        qDebug() << "amnezia-tor onions" << stdOut;

        QString onion = stdOut;
        onion.replace("\n", "");
        protocol.insert(config_key::site, onion);

        containerConfig.insert(ProtocolProps::protoToString(mainProto), protocol);
    }
}

