#include "serverConfig.h"

#include <QJsonArray>

#include "apiV1ServerConfig.h"
#include "apiV2ServerConfig.h"
#include "core/models/containers/containers_defs.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "protocols/protocols_defs.h"
#include "selfHostedServerConfig.h"

using namespace amnezia;

ServerConfig::ServerConfig(const QJsonObject &serverConfigObject)
{
    type = static_cast<ServerConfigType>(serverConfigObject.value(config_key::configVersion).toInt(0));

    hostName = serverConfigObject.value(config_key::hostName).toString();

    dns1 = serverConfigObject.value(config_key::dns1).toString();
    dns2 = serverConfigObject.value(config_key::dns2).toString();

    defaultContainer = serverConfigObject.value(config_key::defaultContainer).toString();

    crc = serverConfigObject.value(config_key::crc).toInt(0);
    nameOverriddenByUser = serverConfigObject.value(config_key::nameOverriddenByUser).toBool(false);

    auto containers = serverConfigObject.value(config_key::containers).toArray();
    for (const auto &container : std::as_const(containers)) {
        auto containerObject = container.toObject();

        auto containerName = containerObject.value(config_key::container).toString();

        ContainerConfig containerConfig;
        containerConfig.containerName = containerName;

        auto protocols = ContainerProps::protocolsForContainer(ContainerProps::containerFromString(containerName));
        for (const auto &protocol : protocols) {
            auto protocolName = ProtocolProps::protoToString(protocol);
            auto protocolConfigObject = containerObject.value(protocolName).toObject();

            switch (protocol) {
            case Proto::OpenVpn: {
                containerConfig.protocolConfigs.insert(protocolName,
                                                       QSharedPointer<OpenVpnProtocolConfig>::create(protocolConfigObject, protocolName));
                break;
            }
            case Proto::ShadowSocks: {
                containerConfig.protocolConfigs.insert(
                        protocolName, QSharedPointer<ShadowsocksProtocolConfig>::create(protocolConfigObject, protocolName));
                break;
            }
            case Proto::Cloak: {
                containerConfig.protocolConfigs.insert(protocolName,
                                                       QSharedPointer<CloakProtocolConfig>::create(protocolConfigObject, protocolName));
                break;
            }
            case Proto::WireGuard: {
                containerConfig.protocolConfigs.insert(protocolName,
                                                       QSharedPointer<WireGuardProtocolConfig>::create(protocolConfigObject, protocolName));
                break;
            }
            case Proto::Awg: {
                containerConfig.protocolConfigs.insert(protocolName,
                                                       QSharedPointer<AwgProtocolConfig>::create(protocolConfigObject, protocolName));
                break;
            }
            case Proto::Xray: {
                containerConfig.protocolConfigs.insert(protocolName,
                                                       QSharedPointer<XrayProtocolConfig>::create(protocolConfigObject, protocolName));
                break;
            }
            case Proto::Ikev2: break;
            case Proto::L2tp: break;
            case Proto::SSXray: break;
            case Proto::TorWebSite: break;
            case Proto::Dns: break;
            case Proto::Sftp: break;
            case Proto::Socks5Proxy: break;
            default: break;
            }
        }

        containerConfigs.insert(containerName, containerConfig);
    }
}

QSharedPointer<ServerConfig> ServerConfig::createServerConfig(const QJsonObject &serverConfigObject)
{
    auto type = static_cast<ServerConfigType>(serverConfigObject.value(config_key::configVersion).toInt(0));

    switch (type) {
    case ServerConfigType::SelfHosted: return QSharedPointer<SelfHostedServerConfig>::create(serverConfigObject);
    case ServerConfigType::ApiV1: return QSharedPointer<ApiV1ServerConfig>::create(serverConfigObject);
    case ServerConfigType::ApiV2: return QSharedPointer<ApiV2ServerConfig>::create(serverConfigObject);
    }
}

ServerConfigVariant ServerConfig::getServerConfigVariant(const QSharedPointer<ServerConfig> &serverConfig)
{
    ServerConfigVariant variant;
    switch (serverConfig->type) {
    case amnezia::ServerConfigType::SelfHosted: variant = qSharedPointerCast<SelfHostedServerConfig>(serverConfig); break;
    case amnezia::ServerConfigType::ApiV1: variant = qSharedPointerCast<ApiV1ServerConfig>(serverConfig); break;
    case amnezia::ServerConfigType::ApiV2: variant = qSharedPointerCast<ApiV2ServerConfig>(serverConfig); break;
    }
    return variant;
}

QJsonObject ServerConfig::toJson() const
{
    QJsonObject json;

    if (type != ServerConfigType::SelfHosted) {
        json[config_key::configVersion] = static_cast<int>(type);
    }

    if (!hostName.isEmpty()) {
        json[config_key::hostName] = hostName;
    }
    if (!dns1.isEmpty()) {
        json[config_key::dns1] = dns1;
    }
    if (!dns2.isEmpty()) {
        json[config_key::dns2] = dns2;
    }
    if (!defaultContainer.isEmpty()) {
        json[config_key::defaultContainer] = defaultContainer;
    }

    if (!containerConfigs.isEmpty()) {
        QJsonArray containersArray;
        for (const auto &containerConfig : containerConfigs) {
            QJsonObject containerObject;
            containerObject[config_key::container] = containerConfig.containerName;

            if (!containerConfig.protocolConfigs.isEmpty()) {
                for (const auto &protocolConfig : containerConfig.protocolConfigs) {
                    QJsonObject protocolJson = protocolConfig->toJson();
                    if (!protocolJson.isEmpty()) {
                        containerObject[protocolConfig->protocolName] = protocolJson;
                    }
                }
            }

            containersArray.append(containerObject);
        }
        if (!containersArray.isEmpty()) {
            json[config_key::containers] = containersArray;
        }
    }

    return json;
}

void ServerConfig::updateProtocolConfig(const QString &containerName, const QMap<QString, QSharedPointer<ProtocolConfig>> &protocolConfigs)
{
    if (containerConfigs.contains(containerName)) {
        ContainerConfig &containerConfig = containerConfigs[containerName];
        containerConfig.protocolConfigs = protocolConfigs;
    }
}
