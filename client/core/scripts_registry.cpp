#include "scripts_registry.h"

#include <QDebug>
#include <QFile>
#include <QJsonObject>
#include <QObject>
#include <QLoggingCategory>
#include "core/networkUtilities.h"
#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"

QString amnezia::scriptFolder(amnezia::DockerContainer container)
{
    switch (container) {
    case DockerContainer::OpenVpn: return QLatin1String("openvpn");
    case DockerContainer::Cloak: return QLatin1String("openvpn_cloak");
    case DockerContainer::ShadowSocks: return QLatin1String("openvpn_shadowsocks");
    case DockerContainer::WireGuard: return QLatin1String("wireguard");
    case DockerContainer::Awg2: return QLatin1String("awg");
    case DockerContainer::Awg: return QLatin1String("awg_legacy");
    case DockerContainer::Ipsec: return QLatin1String("ipsec");
    case DockerContainer::Xray: return QLatin1String("xray");

    case DockerContainer::TorWebSite: return QLatin1String("website_tor");
    case DockerContainer::Dns: return QLatin1String("dns");
    case DockerContainer::Sftp: return QLatin1String("sftp");
    case DockerContainer::Socks5Proxy: return QLatin1String("socks5_proxy");
    default: return QString();
    }
}

QString amnezia::scriptName(SharedScriptType type)
{
    switch (type) {
    case SharedScriptType::prepare_host: return QLatin1String("prepare_host.sh");
    case SharedScriptType::install_docker: return QLatin1String("install_docker.sh");
    case SharedScriptType::build_container: return QLatin1String("build_container.sh");
    case SharedScriptType::remove_container: return QLatin1String("remove_container.sh");
    case SharedScriptType::remove_all_containers: return QLatin1String("remove_all_containers.sh");
    case SharedScriptType::setup_host_firewall: return QLatin1String("setup_host_firewall.sh");
    case SharedScriptType::check_connection: return QLatin1String("check_connection.sh");
    case SharedScriptType::check_server_is_busy: return QLatin1String("check_server_is_busy.sh");
    case SharedScriptType::check_user_in_sudo: return QLatin1String("check_user_in_sudo.sh");
    default: return QString();
    }
}

QString amnezia::scriptName(ProtocolScriptType type)
{
    switch (type) {
    case ProtocolScriptType::dockerfile: return QLatin1String("Dockerfile");
    case ProtocolScriptType::run_container: return QLatin1String("run_container.sh");
    case ProtocolScriptType::configure_container: return QLatin1String("configure_container.sh");
    case ProtocolScriptType::container_startup: return QLatin1String("start.sh");
    case ProtocolScriptType::openvpn_template: return QLatin1String("template.ovpn");
    case ProtocolScriptType::wireguard_template: return QLatin1String("template.conf");
    case ProtocolScriptType::awg_template: return QLatin1String("template.conf");
    case ProtocolScriptType::xray_template: return QLatin1String("template.json");
    default: return QString();
    }
}

QString amnezia::scriptData(amnezia::SharedScriptType type)
{
    QString fileName = QString(":/server_scripts/%1").arg(amnezia::scriptName(type));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Warning: script missing" << fileName;
        return "";
    }
    QByteArray ba = file.readAll();
    if (ba.isEmpty()) {
        qDebug() << "Warning: script is empty" << fileName;
    }
    return ba;
}

QString amnezia::scriptData(amnezia::ProtocolScriptType type, DockerContainer container)
{
    QString fileName = QString(":/server_scripts/%1/%2").arg(amnezia::scriptFolder(container), amnezia::scriptName(type));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Warning: script missing" << fileName;
        return "";
    }
    QByteArray data = file.readAll();
    data.replace("\r", "");
    return data;
}

amnezia::ScriptVars amnezia::genBaseVars(const ServerCredentials &credentials, 
                                          DockerContainer container,
                                          const QString &primaryDns,
                                          const QString &secondaryDns)
{
    ScriptVars vars;

    vars.append({ { "$REMOTE_HOST", credentials.hostName } });
    vars.append({ { "$CONTAINER_NAME", ContainerProps::containerToString(container) } });
    vars.append({ { "$DOCKERFILE_FOLDER", "/opt/amnezia/" + ContainerProps::containerToString(container) } });

    QString serverIp = (!ContainerProps::isAwgContainer(container) && container != DockerContainer::WireGuard && container != DockerContainer::Xray)
            ? NetworkUtilities::getIPAddress(credentials.hostName)
            : credentials.hostName;
    if (!serverIp.isEmpty()) {
        vars.append({ { "$SERVER_IP_ADDRESS", serverIp } });
    } else {
        qWarning() << "amnezia::genBaseVars unable to resolve address for credentials.hostName";
    }

    QString dns1 = primaryDns.isEmpty() ? QString("8.8.8.8") : primaryDns;
    QString dns2 = secondaryDns.isEmpty() ? QString("8.8.4.4") : secondaryDns;
    vars.append({ { "$PRIMARY_SERVER_DNS", dns1 } });
    vars.append({ { "$SECONDARY_SERVER_DNS", dns2 } });

    // IPsec vars (constants)
    vars.append({ { "$IPSEC_VPN_L2TP_NET", "192.168.42.0/24" } });
    vars.append({ { "$IPSEC_VPN_L2TP_POOL", "192.168.42.10-192.168.42.250" } });
    vars.append({ { "$IPSEC_VPN_L2TP_LOCAL", "192.168.42.1" } });
    vars.append({ { "$IPSEC_VPN_XAUTH_NET", "192.168.43.0/24" } });
    vars.append({ { "$IPSEC_VPN_XAUTH_POOL", "192.168.43.10-192.168.43.250" } });
    vars.append({ { "$IPSEC_VPN_SHA2_TRUNCBUG", "yes" } });
    vars.append({ { "$IPSEC_VPN_VPN_ANDROID_MTU_FIX", "yes" } });
    vars.append({ { "$IPSEC_VPN_DISABLE_IKEV2", "no" } });
    vars.append({ { "$IPSEC_VPN_DISABLE_L2TP", "no" } });
    vars.append({ { "$IPSEC_VPN_DISABLE_XAUTH", "no" } });
    vars.append({ { "$IPSEC_VPN_C2C_TRAFFIC", "no" } });

    return vars;
}

amnezia::ScriptVars amnezia::genOpenVpnVars(const QJsonObject &containerConfig)
{
    ScriptVars vars;
    const QJsonObject &openvpnConfig = containerConfig.value(ProtocolProps::protoToString(Proto::OpenVpn)).toObject();

    vars.append({ { "$OPENVPN_SUBNET_IP",
                    openvpnConfig.value(config_key::subnet_address).toString(protocols::openvpn::defaultSubnetAddress) } });
    vars.append({ { "$OPENVPN_SUBNET_CIDR", openvpnConfig.value(config_key::subnet_cidr).toString(protocols::openvpn::defaultSubnetCidr) } });
    vars.append({ { "$OPENVPN_SUBNET_MASK", openvpnConfig.value(config_key::subnet_mask).toString(protocols::openvpn::defaultSubnetMask) } });
    vars.append({ { "$OPENVPN_PORT", openvpnConfig.value(config_key::port).toString(protocols::openvpn::defaultPort) } });
    vars.append({ { "$OPENVPN_TRANSPORT_PROTO",
                    openvpnConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto) } });

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    vars.append({ { "$OPENVPN_NCP_DISABLE", isNcpDisabled ? protocols::openvpn::ncpDisableString : "" } });

    vars.append({ { "$OPENVPN_CIPHER", openvpnConfig.value(config_key::cipher).toString(protocols::openvpn::defaultCipher) } });
    vars.append({ { "$OPENVPN_HASH", openvpnConfig.value(config_key::hash).toString(protocols::openvpn::defaultHash) } });

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    vars.append({ { "$OPENVPN_TLS_AUTH", isTlsAuth ? protocols::openvpn::tlsAuthString : "" } });
    if (!isTlsAuth) {
        vars.append({ { "$OPENVPN_TA_KEY", "" } });
    }

    vars.append({ { "$OPENVPN_ADDITIONAL_CLIENT_CONFIG",
                    openvpnConfig.value(config_key::additional_client_config).toString(protocols::openvpn::defaultAdditionalClientConfig) } });
    vars.append({ { "$OPENVPN_ADDITIONAL_SERVER_CONFIG",
                    openvpnConfig.value(config_key::additional_server_config).toString(protocols::openvpn::defaultAdditionalServerConfig) } });

    return vars;
}

amnezia::ScriptVars amnezia::genShadowSocksVars(const QJsonObject &containerConfig)
{
    ScriptVars vars;
    const QJsonObject &ssConfig = containerConfig.value(ProtocolProps::protoToString(Proto::ShadowSocks)).toObject();

    vars.append({ { "$SHADOWSOCKS_SERVER_PORT", ssConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort) } });
    vars.append({ { "$SHADOWSOCKS_LOCAL_PORT",
                    ssConfig.value(config_key::local_port).toString(protocols::shadowsocks::defaultLocalProxyPort) } });
    vars.append({ { "$SHADOWSOCKS_CIPHER", ssConfig.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher) } });

    return vars;
}

amnezia::ScriptVars amnezia::genCloakVars(const QJsonObject &containerConfig)
{
    ScriptVars vars;
    const QJsonObject &cloakConfig = containerConfig.value(ProtocolProps::protoToString(Proto::Cloak)).toObject();

    vars.append({ { "$CLOAK_SERVER_PORT", cloakConfig.value(config_key::port).toString(protocols::cloak::defaultPort) } });
    vars.append({ { "$FAKE_WEB_SITE_ADDRESS", cloakConfig.value(config_key::site).toString(protocols::cloak::defaultRedirSite) } });

    return vars;
}

amnezia::ScriptVars amnezia::genXrayVars(const QJsonObject &containerConfig)
{
    ScriptVars vars;
    const QJsonObject &xrayConfig = containerConfig.value(ProtocolProps::protoToString(Proto::Xray)).toObject();

    vars.append({ { "$XRAY_SITE_NAME", xrayConfig.value(config_key::site).toString(protocols::xray::defaultSite) } });
    vars.append({ { "$XRAY_SERVER_PORT", xrayConfig.value(config_key::port).toString(protocols::xray::defaultPort) } });

    return vars;
}

amnezia::ScriptVars amnezia::genWireGuardVars(const QJsonObject &containerConfig)
{
    ScriptVars vars;
    const QJsonObject &wireguarConfig = containerConfig.value(ProtocolProps::protoToString(Proto::WireGuard)).toObject();

    vars.append({ { "$WIREGUARD_SUBNET_IP",
                    wireguarConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress) } });
    vars.append({ { "$WIREGUARD_SUBNET_CIDR",
                    wireguarConfig.value(config_key::subnet_cidr).toString(protocols::wireguard::defaultSubnetCidr) } });
    vars.append({ { "$WIREGUARD_SUBNET_MASK",
                    wireguarConfig.value(config_key::subnet_mask).toString(protocols::wireguard::defaultSubnetMask) } });
    vars.append({ { "$WIREGUARD_SERVER_PORT", wireguarConfig.value(config_key::port).toString(protocols::wireguard::defaultPort) } });

    return vars;
}

amnezia::ScriptVars amnezia::genAwgVars(const QJsonObject &containerConfig)
{
    ScriptVars vars;
    const QJsonObject &amneziaWireguarConfig = containerConfig.value(ProtocolProps::protoToString(Proto::Awg)).toObject();

    vars.append({ { "$AWG_SUBNET_IP",
                    amneziaWireguarConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress) } });
    vars.append({ { "$AWG_SERVER_PORT", amneziaWireguarConfig.value(config_key::port).toString(protocols::awg::defaultPort) } });
    vars.append({ { "$JUNK_PACKET_COUNT", amneziaWireguarConfig.value(config_key::junkPacketCount).toString() } });
    vars.append({ { "$JUNK_PACKET_MIN_SIZE", amneziaWireguarConfig.value(config_key::junkPacketMinSize).toString() } });
    vars.append({ { "$JUNK_PACKET_MAX_SIZE", amneziaWireguarConfig.value(config_key::junkPacketMaxSize).toString() } });
    vars.append({ { "$INIT_PACKET_JUNK_SIZE", amneziaWireguarConfig.value(config_key::initPacketJunkSize).toString() } });
    vars.append({ { "$RESPONSE_PACKET_JUNK_SIZE", amneziaWireguarConfig.value(config_key::responsePacketJunkSize).toString() } });
    vars.append({ { "$INIT_PACKET_MAGIC_HEADER", amneziaWireguarConfig.value(config_key::initPacketMagicHeader).toString() } });
    vars.append({ { "$RESPONSE_PACKET_MAGIC_HEADER", amneziaWireguarConfig.value(config_key::responsePacketMagicHeader).toString() } });
    vars.append({ { "$UNDERLOAD_PACKET_MAGIC_HEADER", amneziaWireguarConfig.value(config_key::underloadPacketMagicHeader).toString() } });
    vars.append({ { "$TRANSPORT_PACKET_MAGIC_HEADER", amneziaWireguarConfig.value(config_key::transportPacketMagicHeader).toString() } });
    vars.append({ { "$COOKIE_REPLY_PACKET_JUNK_SIZE", amneziaWireguarConfig.value(config_key::cookieReplyPacketJunkSize).toString() } });
    vars.append({ { "$TRANSPORT_PACKET_JUNK_SIZE", amneziaWireguarConfig.value(config_key::transportPacketJunkSize).toString() } });
    vars.append({ { "$SPECIAL_JUNK_1", amneziaWireguarConfig.value(config_key::specialJunk1).toString() } });
    vars.append({ { "$SPECIAL_JUNK_2", amneziaWireguarConfig.value(config_key::specialJunk2).toString() } });
    vars.append({ { "$SPECIAL_JUNK_3", amneziaWireguarConfig.value(config_key::specialJunk3).toString() } });
    vars.append({ { "$SPECIAL_JUNK_4", amneziaWireguarConfig.value(config_key::specialJunk4).toString() } });
    vars.append({ { "$SPECIAL_JUNK_5", amneziaWireguarConfig.value(config_key::specialJunk5).toString() } });

    return vars;
}

amnezia::ScriptVars amnezia::genSftpVars(const QJsonObject &containerConfig)
{
    ScriptVars vars;
    const QJsonObject &sftpConfig = containerConfig.value(ProtocolProps::protoToString(Proto::Sftp)).toObject();

    vars.append({ { "$SFTP_PORT", sftpConfig.value(config_key::port).toString(QString::number(ProtocolProps::defaultPort(Proto::Sftp))) } });
    vars.append({ { "$SFTP_USER", sftpConfig.value(config_key::userName).toString() } });
    vars.append({ { "$SFTP_PASSWORD", sftpConfig.value(config_key::password).toString() } });

    return vars;
}

amnezia::ScriptVars amnezia::genSocks5ProxyVars(const QJsonObject &containerConfig)
{
    ScriptVars vars;
    const QJsonObject &socks5ProxyConfig = containerConfig.value(ProtocolProps::protoToString(Proto::Socks5Proxy)).toObject();

    vars.append({ { "$SOCKS5_PROXY_PORT", socks5ProxyConfig.value(config_key::port).toString(protocols::socks5Proxy::defaultPort) } });
    auto username = socks5ProxyConfig.value(config_key::userName).toString();
    auto password = socks5ProxyConfig.value(config_key::password).toString();
    QString socks5user = (!username.isEmpty() && !password.isEmpty()) ? QString("users %1:CL:%2").arg(username, password) : "";
    vars.append({ { "$SOCKS5_USER", socks5user } });
    vars.append({ { "$SOCKS5_AUTH_TYPE", socks5user.isEmpty() ? "none" : "strong" } });

    return vars;
}

amnezia::ScriptVars amnezia::genProtocolVarsForContainer(DockerContainer container, const QJsonObject &containerConfig)
{
    ScriptVars vars;
    QVector<Proto> protocols = ContainerProps::protocolsForContainer(container);

    for (Proto protocol : protocols) {
        switch (protocol) {
        case Proto::OpenVpn:
            vars.append(genOpenVpnVars(containerConfig));
            break;
        case Proto::ShadowSocks:
            vars.append(genShadowSocksVars(containerConfig));
            break;
        case Proto::Cloak:
            vars.append(genCloakVars(containerConfig));
            break;
        case Proto::Xray:
            vars.append(genXrayVars(containerConfig));
            break;
        case Proto::WireGuard:
            vars.append(genWireGuardVars(containerConfig));
            break;
        case Proto::Awg:
            vars.append(genAwgVars(containerConfig));
            break;
        case Proto::Sftp:
            vars.append(genSftpVars(containerConfig));
            break;
        case Proto::Socks5Proxy:
            vars.append(genSocks5ProxyVars(containerConfig));
            break;
        default:
            break;
        }
    }

    return vars;
}
