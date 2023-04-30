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
constexpr char adminUser[] = "adminUser";
constexpr char adminPassword[] = "adminPassword";

constexpr char port[] = "port";
constexpr char local_port[] = "local_port";

constexpr char dns1[] = "dns1";
constexpr char dns2[] = "dns2";


constexpr char description[] = "description";
constexpr char cert[] = "cert";
constexpr char config[] = "config";


constexpr char containers[] = "containers";
constexpr char container[] = "container";
constexpr char defaultContainer[] = "defaultContainer";

constexpr char vpnproto[] = "protocol";
constexpr char protocols[] = "protocols";

constexpr char remote[] = "remote";
constexpr char transport_proto[] = "transport_proto";
constexpr char cipher[] = "cipher";
constexpr char hash[] = "hash";
constexpr char ncp_disable[] = "ncp_disable";
constexpr char tls_auth[] = "tls_auth";

constexpr char client_priv_key[] = "client_priv_key";
constexpr char client_pub_key[] = "client_pub_key";
constexpr char server_priv_key[] = "server_priv_key";
constexpr char server_pub_key[] = "server_pub_key";
constexpr char psk_key[] = "psk_key";

constexpr char client_ip[] = "client_ip"; // internal ip address

constexpr char site[] = "site";
constexpr char block_outside_dns[] = "block_outside_dns";

constexpr char subnet_address[] = "subnet_address";
constexpr char subnet_mask[] = "subnet_mask";
constexpr char subnet_cidr[] = "subnet_cidr";

constexpr char additional_client_config[] = "additional_client_config";
constexpr char additional_server_config[] = "additional_server_config";

// proto config keys
constexpr char last_config[] = "last_config";

constexpr char isThirdPartyConfig[] = "isThirdPartyConfig";

constexpr char openvpn[] = "openvpn";
constexpr char wireguard[] = "wireguard";

}

namespace protocols {

namespace dns {
constexpr char amneziaDnsIp[] = "172.29.172.254";
}

namespace openvpn {
constexpr char defaultSubnetAddress[] = "10.8.0.0";
constexpr char defaultSubnetMask[] = "255.255.255.0";
constexpr char defaultSubnetCidr[] = "24";

constexpr char serverConfigPath[] = "/opt/amnezia/openvpn/server.conf";
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

constexpr char defaultAdditionalClientConfig[] = "";
constexpr char defaultAdditionalServerConfig[] = "";
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
constexpr char defaultCipher[] = "chacha20-poly1305";

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

namespace nextcloud {
constexpr char defaultAdminUser[] = "admin";
constexpr char defaultAdminPassword[] = "admin";
}


namespace sftp {
constexpr char defaultUserName[] = "sftp_user";

} // namespace sftp

} // namespace protocols

namespace ProtocolEnumNS {
Q_NAMESPACE

enum TransportProto {
    Udp,
    Tcp
};
Q_ENUM_NS(TransportProto)

enum Proto {
    Any = 0,
    OpenVpn,
    ShadowSocks,
    Cloak,
    WireGuard,
    Ikev2,
    L2tp,

    // non-vpn
    TorWebSite,
    Dns,
    FileShare,
    // Fileshare
    Sftp,
    Nextcloud
};
Q_ENUM_NS(Proto)

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
    Q_INVOKABLE static QList<Proto> allProtocols();

    // spelling may differ for various protocols - TCP for OpenVPN, tcp for others
    Q_INVOKABLE static TransportProto transportProtoFromString(QString p);
    Q_INVOKABLE static QString transportProtoToString(TransportProto proto, Proto p = Proto::Any);

    Q_INVOKABLE static Proto protoFromString(QString p);
    Q_INVOKABLE static QString protoToString(Proto p);

    Q_INVOKABLE static QMap<Proto, QString> protocolHumanNames();
    Q_INVOKABLE static QMap<Proto, QString> protocolDescriptions();

    Q_INVOKABLE static ServiceType protocolService(Proto p);

    Q_INVOKABLE static int defaultPort(Proto p);
    Q_INVOKABLE static bool defaultPortChangeable(Proto p);

    Q_INVOKABLE static TransportProto defaultTransportProto(Proto p);
    Q_INVOKABLE static bool defaultTransportProtoChangeable(Proto p);


    Q_INVOKABLE static QString key_proto_config_data(Proto p);
    Q_INVOKABLE static QString key_proto_config_path(Proto p);

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

    qmlRegisterUncreatableMetaObject(
                ProtocolEnumNS::staticMetaObject,
                "ProtocolEnum",
                1, 0,
                "ServiceType",
                "Error: only enums"
                );
}

} // namespace amnezia

QDebug operator<<(QDebug debug, const amnezia::Proto &p);

#endif // PROTOCOLS_DEFS_H
