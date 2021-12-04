#include "vpn_configurator.h"
#include "openvpn_configurator.h"
#include "cloak_configurator.h"
#include "shadowsocks_configurator.h"
#include "wireguard_configurator.h"
#include "ikev2_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "containers/containers_defs.h"

Settings &VpnConfigurator::m_settings()
{
    static Settings s;
    return s;
}

QString VpnConfigurator::genVpnProtocolConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, Proto proto, ErrorCode *errorCode)
{
    switch (proto) {
    case Proto::OpenVpn:
        return OpenVpnConfigurator::genOpenVpnConfig(credentials, container, containerConfig, errorCode);

    case Proto::ShadowSocks:
        return ShadowSocksConfigurator::genShadowSocksConfig(credentials, container, containerConfig, errorCode);

    case Proto::Cloak:
        return CloakConfigurator::genCloakConfig(credentials, container, containerConfig, errorCode);

    case Proto::WireGuard:
        return WireguardConfigurator::genWireguardConfig(credentials, container, containerConfig, errorCode);

    case Proto::Ikev2:
        return Ikev2Configurator::genIkev2Config(credentials, container, containerConfig, errorCode);

    default:
        return "";
    }
}

QString VpnConfigurator::processConfigWithLocalSettings(DockerContainer container, Proto proto, QString config)
{
    config.replace("$PRIMARY_DNS", m_settings().primaryDns());
    config.replace("$SECONDARY_DNS", m_settings().secondaryDns());

    if (proto == Proto::OpenVpn) {
        return OpenVpnConfigurator::processConfigWithLocalSettings(config);
    }
    return config;
}

QString VpnConfigurator::processConfigWithExportSettings(DockerContainer container, Proto proto, QString config)
{
    config.replace("$PRIMARY_DNS", m_settings().primaryDns());
    config.replace("$SECONDARY_DNS", m_settings().secondaryDns());

    if (proto == Proto::OpenVpn) {
        return OpenVpnConfigurator::processConfigWithExportSettings(config);
    }
    return config;
}

void VpnConfigurator::updateContainerConfigAfterInstallation(DockerContainer container, QJsonObject &containerConfig,
    const QString &stdOut)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    if (container == DockerContainer::TorWebSite) {
        QJsonObject protocol = containerConfig.value(ProtocolProps::protoToString(mainProto)).toObject();

        qDebug() << "amnezia-tor onions" << stdOut;

        QStringList l = stdOut.split(",");
        for (QString s : l) {
            if (s.contains(":80")) {
                protocol.insert(config_key::site, s);
            }
        }

        containerConfig.insert(ProtocolProps::protoToString(mainProto), protocol);
    }
}
