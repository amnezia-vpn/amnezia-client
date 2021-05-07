#ifndef PROTOCOLS_DEFS_H
#define PROTOCOLS_DEFS_H

#include <QObject>

namespace amnezia {
namespace config_key {

// Json config strings
constexpr char hostName[] = "hostName";
constexpr char userName[] = "userName";
constexpr char password[] = "password";
constexpr char port[] = "port";
constexpr char local_port[] = "local_port";

constexpr char description[] = "description";


constexpr char containers[] = "containers";
constexpr char container[] = "container";
constexpr char defaultContainer[] = "defaultContainer";

constexpr char protocols[] = "protocols";
//constexpr char protocol[] = "protocol";

constexpr char remote[] = "remote";
constexpr char transport_proto[] = "transport_proto";
constexpr char cipher[] = "cipher";
constexpr char hash[] = "hash";
constexpr char ncp_disable[] = "ncp_disable";

constexpr char site[] = "site";
constexpr char block_outside_dns[] = "block_outside_dns";

constexpr char subnet_address[] = "subnet_address";
constexpr char subnet_mask[] = "subnet_mask";
constexpr char subnet_mask_val[] = "subnet_mask_val";

// proto config keys
constexpr char last_config[] = "last_config";

constexpr char openvpn[] = "openvpn";
constexpr char shadowsocks[] = "shadowsocks";
constexpr char cloak[] = "cloak";
constexpr char wireguard[] = "wireguard";

// containers config keys
constexpr char amnezia_openvpn[] = "amnezia-openvpn";
constexpr char amnezia_shadowsocks[] = "amnezia-shadowsocks";
constexpr char amnezia_openvpn_cloak[] = "amnezia-openvpn-cloak";
constexpr char amnezia_wireguard[] = "amnezia-wireguard";
}

namespace protocols {

constexpr char vpnDefaultSubnetAddress[] = "10.8.0.0";
constexpr char vpnDefaultSubnetMask[] = "255.255.255.0";
constexpr char vpnDefaultSubnetMaskVal[] = "24";

constexpr char UDP[] = "udp"; // case sens
constexpr char TCP[] = "tcp";

namespace openvpn {
constexpr char caCertPath[] = "/opt/amnezia/openvpn/pki/ca.crt";
constexpr char clientCertPath[] = "/opt/amnezia/openvpn/pki/issued";
constexpr char taKeyPath[] = "/opt/amnezia/openvpn/ta.key";
constexpr char clientsDirPath[] = "/opt/amnezia/openvpn/clients";
constexpr char defaultPort[] = "1194";
constexpr char defaultTransportProto[] = amnezia::protocols::UDP;
constexpr char defaultCipher[] = "AES-256-GCM";
constexpr char defaultHash[] = "SHA512";
constexpr bool defaultBlockOutsideDns = true;
constexpr bool defaultNcpDisable = false;
constexpr char ncpDisableString[] = "ncp-disable";

}

namespace shadowsocks {
constexpr char ssKeyPath[] = "/opt/amnezia/shadowsocks/shadowsocks.key";
constexpr char defaultPort[] = "6789";
constexpr char defaultLocalProxyPort[] = "8585";
constexpr char defaultCipher[] = "chacha20-ietf-poly1305";
}

namespace cloak {
constexpr char ckPublicKeyPath[] = "/opt/amnezia/cloak/cloak_public.key";
constexpr char ckBypassUidKeyPath[] = "/opt/amnezia/cloak/cloak_bypass_uid.key";
constexpr char ckAdminKeyPath[] = "/opt/amnezia/cloak/cloak_admin_uid.key";
constexpr char defaultPort[] = "443";
constexpr char defaultRedirSite[] = "mail.ru";
}



} // namespace protocols

enum class Protocol {
    Any,
    OpenVpn,
    ShadowSocks,
    Cloak,
    WireGuard
};

inline Protocol protoFromString(QString proto){
    if (proto == config_key::openvpn) return Protocol::OpenVpn;
    if (proto == config_key::cloak) return Protocol::Cloak;
    if (proto == config_key::shadowsocks) return Protocol::ShadowSocks;
    if (proto == config_key::wireguard) return Protocol::WireGuard;
    return Protocol::Any;
}

inline QString protoToString(Protocol proto){
    switch (proto) {
    case(Protocol::OpenVpn): return config_key::openvpn;
    case(Protocol::Cloak): return config_key::cloak;
    case(Protocol::ShadowSocks): return config_key::shadowsocks;
    case(Protocol::WireGuard): return config_key::wireguard;
    default: return "";
    }
}

enum class DockerContainer {
    None,
    OpenVpn,
    OpenVpnOverShadowSocks,
    OpenVpnOverCloak,
    WireGuard
};

inline DockerContainer containerFromString(const QString &container){
    if (container == config_key::amnezia_openvpn) return DockerContainer::OpenVpn;
    if (container == config_key::amnezia_openvpn_cloak) return DockerContainer::OpenVpnOverCloak;
    if (container == config_key::amnezia_shadowsocks) return DockerContainer::OpenVpnOverShadowSocks;
    if (container == config_key::amnezia_wireguard) return DockerContainer::WireGuard;
    return DockerContainer::None;
}

inline QString containerToString(DockerContainer container){
    switch (container) {
    case(DockerContainer::OpenVpn): return config_key::amnezia_openvpn;
    case(DockerContainer::OpenVpnOverCloak): return config_key::amnezia_openvpn_cloak;
    case(DockerContainer::OpenVpnOverShadowSocks): return config_key::amnezia_shadowsocks;
    case(DockerContainer::WireGuard): return config_key::amnezia_wireguard;
    default: return "";
    }
}

} // namespace amnezia

#endif // PROTOCOLS_DEFS_H
