#ifndef PROTOCOLS_DEFS_H
#define PROTOCOLS_DEFS_H

#include <QObject>
#include <QDebug>
#include <QQmlEngine>

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
constexpr char tls_auth[] = "tls_auth";

constexpr char site[] = "site";
constexpr char block_outside_dns[] = "block_outside_dns";

constexpr char subnet_address[] = "subnet_address";
constexpr char subnet_mask[] = "subnet_mask";
constexpr char subnet_cidr[] = "subnet_cidr";

// proto config keys
constexpr char last_config[] = "last_config";
}

namespace protocols {



constexpr char UDP[] = "udp"; // case sens
constexpr char TCP[] = "tcp";

namespace openvpn {
constexpr char defaultSubnetAddress[] = "10.8.0.0";
constexpr char defaultSubnetMask[] = "255.255.255.0";
constexpr char defaultSubnetCidr[] = "24";

constexpr char caCertPath[] = "/opt/amnezia/openvpn/pki/ca.crt";
constexpr char clientCertPath[] = "/opt/amnezia/openvpn/pki/issued";
constexpr char taKeyPath[] = "/opt/amnezia/openvpn/ta.key";
constexpr char clientsDirPath[] = "/opt/amnezia/openvpn/clients";
constexpr char defaultPort[] = "1194";
constexpr char defaultTransportProto[] = "udp";
constexpr char defaultCipher[] = "AES-256-GCM";
constexpr char defaultHash[] = "SHA512";
constexpr bool defaultBlockOutsideDns = true;
constexpr bool defaultNcpDisable = false;
constexpr bool defaultTlsAuth = true;
constexpr char ncpDisableString[] = "ncp-disable";
constexpr char tlsAuthString[] = "tls-auth /opt/amnezia/openvpn/ta.key 0";

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
constexpr char defaultRedirSite[] = "tile.openstreetmap.org";
constexpr char defaultCipher[] = "chacha20-ietf-poly1305";

}

namespace wireguard {
constexpr char defaultSubnetAddress[] = "10.8.1.0";
constexpr char defaultSubnetMask[] = "255.255.255.0";
constexpr char defaultSubnetCidr[] = "24";

constexpr char defaultPort[] = "51820";
constexpr char serverConfigPath[] = "/opt/amnezia/wireguard/wg0.conf";
constexpr char serverPublicKeyPath[] = "/opt/amnezia/wireguard/wireguard_server_public_key.key";
constexpr char serverPskKeyPath[] = "/opt/amnezia/wireguard/wireguard_psk.key";

}


} // namespace protocols

namespace ProtocolEnumNS {
Q_NAMESPACE

enum TransportProto {
    Udp,
    Tcp
};
Q_ENUM_NS(TransportProto)

enum Protocol {
    Any = 0,
    OpenVpn,
    ShadowSocks,
    Cloak,
    WireGuard,
    TorSite,
    Dns,
    FileShare
};
Q_ENUM_NS(Protocol)

enum ServiceType {
    None = 0,
    Vpn,
    Other
};
Q_ENUM_NS(ServiceType)
} // namespace ProtocolEnumNS

using namespace ProtocolEnumNS;

class ProtocolProps : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE static QList<Protocol> allProtocols();

    // spelling may differ for various protocols - TCP for OpenVPN, tcp for others
    Q_INVOKABLE static TransportProto transportProtoFromString(QString p);
    Q_INVOKABLE static QString transportProtoToString(TransportProto proto, Protocol p = Protocol::Any);

    Q_INVOKABLE static Protocol protoFromString(QString p);
    Q_INVOKABLE static QString protoToString(Protocol p);

    Q_INVOKABLE static QMap<Protocol, QString> protocolHumanNames();
    Q_INVOKABLE static QMap<Protocol, QString> protocolDescriptions();

    Q_INVOKABLE static ServiceType protocolService(Protocol p);

    Q_INVOKABLE static int defaultPort(Protocol p);
    Q_INVOKABLE static bool defaultPortChangeable(Protocol p);

    Q_INVOKABLE static TransportProto defaultTransportProto(Protocol p);
    Q_INVOKABLE static bool defaultTransportProtoChangeable(Protocol p);

};

static void declareQmlProtocolEnum() {
    qmlRegisterUncreatableMetaObject(
                ProtocolEnumNS::staticMetaObject,
                "ProtocolEnum",
                1, 0,
                "ProtocolEnum",
                "Error: only enums"
                );

    qmlRegisterUncreatableMetaObject(
                ProtocolEnumNS::staticMetaObject,
                "ProtocolEnum",
                1, 0,
                "TransportProto",
                "Error: only enums"
                );
}

} // namespace amnezia

QDebug operator<<(QDebug debug, const amnezia::Protocol &p);

#endif // PROTOCOLS_DEFS_H
