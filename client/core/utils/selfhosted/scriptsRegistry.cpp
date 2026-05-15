#include "scriptsRegistry.h"

#include <QDebug>
#include <QFile>
#include <QJsonObject>
#include <QObject>
#include <QLoggingCategory>
#include "core/utils/networkUtilities.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/openVpnProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/protocols/sftpProtocolConfig.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

QString amnezia::scriptFolder(amnezia::DockerContainer container)
{
    switch (container) {
    case DockerContainer::OpenVpn: return QLatin1String("openvpn");
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

QString amnezia::scriptName(ClientScriptType type)
{
    switch (type) {
    case ClientScriptType::mac_installer: return QLatin1String("mac_installer.sh");
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

QString amnezia::scriptData(ClientScriptType type)
{
    QString fileName = QString(":/client_scripts/%1").arg(amnezia::scriptName(type));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Warning: script missing" << fileName;
        return "";
    }
    QByteArray data = file.readAll();
    if (data.isEmpty()) {
        qDebug() << "Warning: script is empty" << fileName;
    }
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
    vars.append({ { "$CONTAINER_NAME", ContainerUtils::containerToString(container) } });
    vars.append({ { "$DOCKERFILE_FOLDER", "/opt/amnezia/" + ContainerUtils::containerToString(container) } });

    QString serverIp = (!ContainerUtils::isAwgContainer(container) && container != DockerContainer::WireGuard && container != DockerContainer::Xray)
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

amnezia::ScriptVars amnezia::genOpenVpnVars(const ContainerConfig &containerConfig)
{
    ScriptVars vars;
    
    if (auto* openVpnProtocolConfig = containerConfig.getOpenVpnProtocolConfig()) {
        const OpenVpnServerConfig& config = openVpnProtocolConfig->serverConfig;
        
        vars.append({ { "$OPENVPN_SUBNET_IP", config.subnetAddress.isEmpty() ? protocols::openvpn::defaultSubnetAddress : config.subnetAddress } });
        vars.append({ { "$OPENVPN_SUBNET_CIDR", config.subnetCidr.isEmpty() ? protocols::openvpn::defaultSubnetCidr : config.subnetCidr } });
        vars.append({ { "$OPENVPN_SUBNET_MASK", config.subnetMask.isEmpty() ? protocols::openvpn::defaultSubnetMask : config.subnetMask } });
        vars.append({ { "$OPENVPN_PORT", config.port.isEmpty() ? protocols::openvpn::defaultPort : config.port } });
        vars.append({ { "$OPENVPN_TRANSPORT_PROTO", config.transportProto.isEmpty() ? protocols::openvpn::defaultTransportProto : config.transportProto } });
        
        vars.append({ { "$OPENVPN_NCP_DISABLE", config.ncpDisable ? protocols::openvpn::ncpDisableString : "" } });
        vars.append({ { "$OPENVPN_CIPHER", config.cipher.isEmpty() ? protocols::openvpn::defaultCipher : config.cipher } });
        vars.append({ { "$OPENVPN_HASH", config.hash.isEmpty() ? protocols::openvpn::defaultHash : config.hash } });
        
        vars.append({ { "$OPENVPN_TLS_AUTH", config.tlsAuth ? protocols::openvpn::tlsAuthString : "" } });
        if (!config.tlsAuth) {
            vars.append({ { "$OPENVPN_TA_KEY", "" } });
        }
        
        vars.append({ { "$OPENVPN_ADDITIONAL_CLIENT_CONFIG", config.additionalClientConfig.isEmpty() ? protocols::openvpn::defaultAdditionalClientConfig : config.additionalClientConfig } });
        vars.append({ { "$OPENVPN_ADDITIONAL_SERVER_CONFIG", config.additionalServerConfig.isEmpty() ? protocols::openvpn::defaultAdditionalServerConfig : config.additionalServerConfig } });
    }
    
    return vars;
}

amnezia::ScriptVars amnezia::genXrayVars(const ContainerConfig &containerConfig)
{
    ScriptVars vars;
    
    if (auto* xrayProtocolConfig = containerConfig.getXrayProtocolConfig()) {
        const XrayServerConfig& config = xrayProtocolConfig->serverConfig;
        
        vars.append({ { "$XRAY_SITE_NAME", config.site.isEmpty() ? protocols::xray::defaultSite : config.site } });
        vars.append({ { "$XRAY_SERVER_PORT", config.port.isEmpty() ? protocols::xray::defaultPort : config.port } });
    }
    
    return vars;
}

amnezia::ScriptVars amnezia::genWireGuardVars(const ContainerConfig &containerConfig)
{
    ScriptVars vars;
    
    if (auto* wireGuardProtocolConfig = containerConfig.getWireGuardProtocolConfig()) {
        const WireGuardServerConfig& config = wireGuardProtocolConfig->serverConfig;
        
        vars.append({ { "$WIREGUARD_SUBNET_IP", config.subnetAddress.isEmpty() ? protocols::wireguard::defaultSubnetAddress : config.subnetAddress } });
        vars.append({ { "$WIREGUARD_SUBNET_CIDR", config.subnetCidr.isEmpty() ? protocols::wireguard::defaultSubnetCidr : config.subnetCidr } });
        vars.append({ { "$WIREGUARD_SERVER_PORT", config.port.isEmpty() ? protocols::wireguard::defaultPort : config.port } });
    }
    
    return vars;
}

amnezia::ScriptVars amnezia::genAwgVars(const ContainerConfig &containerConfig)
{
    ScriptVars vars;
    
    if (auto* awgProtocolConfig = containerConfig.getAwgProtocolConfig()) {
        const AwgServerConfig& config = awgProtocolConfig->serverConfig;
        
        vars.append({ { "$AWG_SUBNET_IP", config.subnetAddress.isEmpty() ? protocols::wireguard::defaultSubnetAddress : config.subnetAddress } });
        vars.append({ { "$WIREGUARD_SUBNET_CIDR", config.subnetCidr.isEmpty() ? protocols::wireguard::defaultSubnetCidr : config.subnetCidr } });
        vars.append({ { "$AWG_SERVER_PORT", config.port.isEmpty() ? protocols::awg::defaultPort : config.port } });
        vars.append({ { "$JUNK_PACKET_COUNT", config.junkPacketCount } });
        vars.append({ { "$JUNK_PACKET_MIN_SIZE", config.junkPacketMinSize } });
        vars.append({ { "$JUNK_PACKET_MAX_SIZE", config.junkPacketMaxSize } });
        vars.append({ { "$INIT_PACKET_JUNK_SIZE", config.initPacketJunkSize } });
        vars.append({ { "$RESPONSE_PACKET_JUNK_SIZE", config.responsePacketJunkSize } });
        vars.append({ { "$INIT_PACKET_MAGIC_HEADER", config.initPacketMagicHeader } });
        vars.append({ { "$RESPONSE_PACKET_MAGIC_HEADER", config.responsePacketMagicHeader } });
        vars.append({ { "$UNDERLOAD_PACKET_MAGIC_HEADER", config.underloadPacketMagicHeader } });
        vars.append({ { "$TRANSPORT_PACKET_MAGIC_HEADER", config.transportPacketMagicHeader } });
        vars.append({ { "$COOKIE_REPLY_PACKET_JUNK_SIZE", config.cookieReplyPacketJunkSize } });
        vars.append({ { "$TRANSPORT_PACKET_JUNK_SIZE", config.transportPacketJunkSize } });
        vars.append({ { "$SPECIAL_JUNK_1", config.specialJunk1 } });
        vars.append({ { "$SPECIAL_JUNK_2", config.specialJunk2 } });
        vars.append({ { "$SPECIAL_JUNK_3", config.specialJunk3 } });
        vars.append({ { "$SPECIAL_JUNK_4", config.specialJunk4 } });
        vars.append({ { "$SPECIAL_JUNK_5", config.specialJunk5 } });
    }
    
    return vars;
}

amnezia::ScriptVars amnezia::genSftpVars(const ContainerConfig &containerConfig)
{
    ScriptVars vars;
    
    if (auto* sftpProtocolConfig = containerConfig.getSftpProtocolConfig()) {
        vars.append({ { "$SFTP_PORT", sftpProtocolConfig->port.isEmpty() ? QString::number(ProtocolUtils::defaultPort(Proto::Sftp)) : sftpProtocolConfig->port } });
        vars.append({ { "$SFTP_USER", sftpProtocolConfig->userName } });
        vars.append({ { "$SFTP_PASSWORD", sftpProtocolConfig->password } });
    }
    
    return vars;
}

amnezia::ScriptVars amnezia::genSocks5ProxyVars(const ContainerConfig &containerConfig)
{
    ScriptVars vars;
    
    if (auto* socks5ProxyProtocolConfig = containerConfig.getSocks5ProxyProtocolConfig()) {
        vars.append({ { "$SOCKS5_PROXY_PORT", socks5ProxyProtocolConfig->port.isEmpty() ? protocols::socks5Proxy::defaultPort : socks5ProxyProtocolConfig->port } });
        QString socks5user = (!socks5ProxyProtocolConfig->userName.isEmpty() && !socks5ProxyProtocolConfig->password.isEmpty()) 
            ? QString("users %1:CL:%2").arg(socks5ProxyProtocolConfig->userName, socks5ProxyProtocolConfig->password) 
            : "";
        vars.append({ { "$SOCKS5_USER", socks5user } });
        vars.append({ { "$SOCKS5_AUTH_TYPE", socks5user.isEmpty() ? "none" : "strong" } });
    }
    
    return vars;
}

amnezia::ScriptVars amnezia::genProtocolVarsForContainer(DockerContainer container, const ContainerConfig &containerConfig)
{
    ScriptVars vars;
    Proto protocol = ContainerUtils::defaultProtocol(container);

    switch (protocol) {
    case Proto::OpenVpn:
        vars.append(genOpenVpnVars(containerConfig));
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

    return vars;
}
