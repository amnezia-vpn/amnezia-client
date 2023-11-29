#include "vpn_configurator.h"
#include "cloak_configurator.h"
#include "ikev2_configurator.h"
#include "openvpn_configurator.h"
#include "shadowsocks_configurator.h"
#include "ssh_configurator.h"
#include "wireguard_configurator.h"
#include "awg_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "settings.h"
#include "utilities.h"

VpnConfigurator::VpnConfigurator(std::shared_ptr<Settings> settings, QObject *parent)
    : ConfiguratorBase(settings, parent)
{
    openVpnConfigurator = std::shared_ptr<OpenVpnConfigurator>(new OpenVpnConfigurator(settings, this));
    shadowSocksConfigurator = std::shared_ptr<ShadowSocksConfigurator>(new ShadowSocksConfigurator(settings, this));
    cloakConfigurator = std::shared_ptr<CloakConfigurator>(new CloakConfigurator(settings, this));
    wireguardConfigurator = std::shared_ptr<WireguardConfigurator>(new WireguardConfigurator(settings, false, this));
    ikev2Configurator = std::shared_ptr<Ikev2Configurator>(new Ikev2Configurator(settings, this));
    sshConfigurator = std::shared_ptr<SshConfigurator>(new SshConfigurator(settings, this));
    awgConfigurator = std::shared_ptr<AwgConfigurator>(new AwgConfigurator(settings, this));
}

QString VpnConfigurator::genVpnProtocolConfig(const ServerCredentials &credentials, DockerContainer container,
                                              const QJsonObject &containerConfig, Proto proto, QString &clientId, ErrorCode *errorCode)
{
    switch (proto) {
    case Proto::OpenVpn:
        return openVpnConfigurator->genOpenVpnConfig(credentials, container, containerConfig, clientId, errorCode);

    case Proto::ShadowSocks:
        return shadowSocksConfigurator->genShadowSocksConfig(credentials, container, containerConfig, errorCode);

    case Proto::Cloak: return cloakConfigurator->genCloakConfig(credentials, container, containerConfig, errorCode);

    case Proto::WireGuard:
        return wireguardConfigurator->genWireguardConfig(credentials, container, containerConfig, clientId, errorCode);

    case Proto::Awg:
        return awgConfigurator->genAwgConfig(credentials, container, containerConfig, clientId, errorCode);

    case Proto::Ikev2: return ikev2Configurator->genIkev2Config(credentials, container, containerConfig, errorCode);

    default: return "";
    }
}

QPair<QString, QString> VpnConfigurator::getDnsForConfig(int serverIndex)
{
    QPair<QString, QString> dns;

    bool useAmneziaDns = m_settings->useAmneziaDns();
    const QJsonObject &server = m_settings->server(serverIndex);

    dns.first = server.value(config_key::dns1).toString();
    dns.second = server.value(config_key::dns2).toString();

    if (dns.first.isEmpty() || !Utils::checkIPv4Format(dns.first)) {
        if (useAmneziaDns && m_settings->containers(serverIndex).contains(DockerContainer::Dns)) {
            dns.first = protocols::dns::amneziaDnsIp;
        } else
            dns.first = m_settings->primaryDns();
    }
    if (dns.second.isEmpty() || !Utils::checkIPv4Format(dns.second)) {
        dns.second = m_settings->secondaryDns();
    }

    qDebug() << "VpnConfigurator::getDnsForConfig" << dns.first << dns.second;
    return dns;
}

QString &VpnConfigurator::processConfigWithDnsSettings(int serverIndex, DockerContainer container, Proto proto,
                                                       QString &config)
{
    auto dns = getDnsForConfig(serverIndex);

    config.replace("$PRIMARY_DNS", dns.first);
    config.replace("$SECONDARY_DNS", dns.second);

    return config;
}

QString &VpnConfigurator::processConfigWithLocalSettings(int serverIndex, DockerContainer container, Proto proto,
                                                         QString &config)
{
    processConfigWithDnsSettings(serverIndex, container, proto, config);

    if (proto == Proto::OpenVpn) {
        config = openVpnConfigurator->processConfigWithLocalSettings(config);
    }
    return config;
}

QString &VpnConfigurator::processConfigWithExportSettings(int serverIndex, DockerContainer container, Proto proto,
                                                          QString &config)
{
    processConfigWithDnsSettings(serverIndex, container, proto, config);

    if (proto == Proto::OpenVpn) {
        config = openVpnConfigurator->processConfigWithExportSettings(config);
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

        QString onion = stdOut;
        onion.replace("\n", "");
        protocol.insert(config_key::site, onion);

        containerConfig.insert(ProtocolProps::protoToString(mainProto), protocol);
    }
}
