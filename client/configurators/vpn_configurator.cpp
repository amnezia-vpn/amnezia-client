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
#include "utils.h"

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

QPair<QString, QString> VpnConfigurator::getDnsForConfig(int serverIndex)
{
    QPair<QString, QString> dns;

    bool useAmneziaDns = m_settings().useAmneziaDns();
    const QJsonObject &server = m_settings().server(serverIndex);

    dns.first = server.value(config_key::dns1).toString();
    dns.second = server.value(config_key::dns2).toString();

    if (dns.first.isEmpty() || !Utils::checkIPv4Format(dns.first)) {
        if (useAmneziaDns && m_settings().containers(serverIndex).contains(DockerContainer::Dns)) {
            dns.first = protocols::dns::amneziaDnsIp;
        }
        else dns.first = m_settings().primaryDns();
    }
    if (dns.second.isEmpty() || !Utils::checkIPv4Format(dns.second)) {
        dns.second = m_settings().secondaryDns();
    }

    qDebug() << "VpnConfigurator::getDnsForConfig" << dns.first << dns.second;
    return dns;
}

QString &VpnConfigurator::processConfigWithDnsSettings(int serverIndex, DockerContainer container,
    Proto proto, QString &config)
{
    auto dns = getDnsForConfig(serverIndex);

    config.replace("$PRIMARY_DNS", dns.first);
    config.replace("$SECONDARY_DNS", dns.second);

    return config;
}

QString &VpnConfigurator::processConfigWithLocalSettings(int serverIndex, DockerContainer container,
    Proto proto, QString &config)
{
    processConfigWithDnsSettings(serverIndex, container, proto, config);

    if (proto == Proto::OpenVpn) {
        config = OpenVpnConfigurator::processConfigWithLocalSettings(config);
    }
    return config;
}

QString &VpnConfigurator::processConfigWithExportSettings(int serverIndex, DockerContainer container,
    Proto proto, QString &config)
{
    processConfigWithDnsSettings(serverIndex, container, proto, config);

    if (proto == Proto::OpenVpn) {
        config = OpenVpnConfigurator::processConfigWithExportSettings(config);
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
