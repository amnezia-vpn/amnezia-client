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
#include "core/models/protocols/torWebsiteProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include <variant>

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

QSharedPointer<ProtocolConfig> VpnConfigurationsController::createProtocolConfig(const Proto protocol)
{
    switch (protocol) {
    case Proto::OpenVpn: 
        return QSharedPointer<OpenVpnProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
    case Proto::ShadowSocks: 
        return QSharedPointer<ShadowsocksProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
    case Proto::Cloak: 
        return QSharedPointer<CloakProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
    case Proto::WireGuard: 
        return QSharedPointer<WireGuardProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
    case Proto::Awg: 
        return QSharedPointer<AwgProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
    case Proto::Xray: 
    case Proto::SSXray: 
        return QSharedPointer<XrayProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
    case Proto::Ikev2: 
        return QSharedPointer<Ikev2ProtocolConfig>::create(ProtocolProps::protoToString(protocol));
    case Proto::TorWebSite:
        return QSharedPointer<TorWebsiteProtocolConfig>::create(QJsonObject(), ProtocolProps::protoToString(protocol));
    default: 
        return nullptr;
    }
}

ErrorCode VpnConfigurationsController::createProtocolConfigForContainer(const ServerCredentials &credentials,
                                                                        const DockerContainer container, ContainerConfig &containerConfig)
{
    ErrorCode errorCode = ErrorCode::NoError;

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return errorCode;
    }

    for (Proto protocol : ContainerProps::protocolsForContainer(container)) {
        QString protocolName = ProtocolProps::protoToString(protocol);
        auto protocolConfig = containerConfig.protocolConfigs.value(protocolName);
        
        if (!protocolConfig) {
            protocolConfig = createProtocolConfig(protocol);
            if (!protocolConfig) {
                errorCode = ErrorCode::InternalError;
                return errorCode;
            }
            containerConfig.protocolConfigs.insert(protocolName, protocolConfig);
        }

        auto configurator = createConfigurator(protocol);
        auto result = configurator->createConfig(credentials, container, protocolConfig, errorCode);
        if (errorCode != ErrorCode::NoError || !result) {
            return errorCode;
        }

        containerConfig.protocolConfigs.insert(protocolName, result);
    }

    return errorCode;
}

ErrorCode VpnConfigurationsController::createProtocolConfigString(const bool isApiConfig, const QPair<QString, QString> &dns,
                                                              const ServerCredentials &credentials, const DockerContainer container,
                                                              const ContainerConfig &containerConfig, const Proto protocol,
                                                              QString &protocolConfigString)
{
    ErrorCode errorCode = ErrorCode::NoError;

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return errorCode;
    }

    QString protocolName = ProtocolProps::protoToString(protocol);
    auto protocolConfig = containerConfig.protocolConfigs.value(protocolName);
    if (!protocolConfig) {
        errorCode = ErrorCode::InternalError;
        return errorCode;
    }

    auto configurator = createConfigurator(protocol);
    auto result = configurator->createConfig(credentials, container, protocolConfig, errorCode);
    if (errorCode != ErrorCode::NoError || !result) {
        return errorCode;
    }

    configurator->processConfigWithExportSettings(dns, isApiConfig, result);
    ProtocolConfigVariant variant = ProtocolConfig::getProtocolConfigVariant(result);
    std::visit([&protocolConfigString](const auto &config) -> void {
        protocolConfigString = config->clientProtocolConfig.nativeConfig;
    }, variant);

    return errorCode;
}

QJsonObject VpnConfigurationsController::createVpnConfiguration(const QPair<QString, QString> &dns, const QSharedPointer<ServerConfig> &serverConfig,
                                                                const ContainerConfig &containerConfig, const DockerContainer container)
{
    QJsonObject vpnConfiguration {};

    if (ContainerProps::containerService(container) == ServiceType::Other) {
        return vpnConfiguration;
    }

    bool isApiConfig = static_cast<int>(serverConfig->type);

    for (ProtocolEnumNS::Proto proto : ContainerProps::protocolsForContainer(container)) {
        if (isApiConfig && container == DockerContainer::Cloak && proto == ProtocolEnumNS::Proto::ShadowSocks) {
            continue;
        }

        QString protocolName = ProtocolProps::protoToString(proto);
        auto protocolConfig = containerConfig.protocolConfigs.value(protocolName);
        QString protocolConfigString;
        
        if (protocolConfig) {
            auto configurator = createConfigurator(proto);
            configurator->processConfigWithLocalSettings(dns, isApiConfig, protocolConfig);
            ProtocolConfigVariant variant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
            std::visit([&protocolConfigString](const auto &config) -> void {
                protocolConfigString = config->clientProtocolConfig.nativeConfig;
            }, variant);
        } else {
            protocolConfigString = "";
        }

        QJsonObject vpnConfigData;
        if (proto == Proto::Xray || proto == Proto::SSXray) {
            vpnConfigData = QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();
        } else {
            vpnConfigData[config_key::config] = protocolConfigString;
            if (protocolConfig) {
                QJsonObject protocolJson = protocolConfig->toJson();
                for (auto it = protocolJson.begin(); it != protocolJson.end(); ++it) {
                    if (it.key() != config_key::config && it.key() != config_key::last_config) {
                        vpnConfigData[it.key()] = it.value();
                    }
                }
            }
        }
        
        if (container == DockerContainer::Awg || container == DockerContainer::WireGuard) {
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

    vpnConfiguration[config_key::hostName] = serverConfig->hostName;
    vpnConfiguration[config_key::description] = serverConfig->toJson().value(config_key::description).toString();

    vpnConfiguration[config_key::configVersion] = static_cast<int>(serverConfig->type);


    return vpnConfiguration;
}

void VpnConfigurationsController::updateContainerConfigAfterInstallation(const DockerContainer container, ContainerConfig &containerConfig,
                                                                        const QString &stdOut)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    if (container == DockerContainer::TorWebSite) {
        QString protocolName = ProtocolProps::protoToString(mainProto);
        auto protocolConfig = containerConfig.protocolConfigs.value(protocolName);

        qDebug() << "amnezia-tor onions" << stdOut;

        QString onion = stdOut;
        onion.replace("\n", "");
        
        if (auto torConfig = qSharedPointerCast<TorWebsiteProtocolConfig>(protocolConfig)) {
            torConfig->serverProtocolConfig.site = onion;
        }
    }
}

