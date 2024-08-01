#ifndef PROTOCOLS_DEFS_H
#define PROTOCOLS_DEFS_H

#include <QDebug>
#include <QMetaEnum>
#include <QObject>

namespace amnezia
{
    namespace config_key
    {

        // Json config strings
        constexpr char hostName[] = "hostName";
        constexpr char userName[] = "userName";
        constexpr char password[] = "password";
        constexpr char port[] = "port";
        constexpr char local_port[] = "local_port";

        constexpr char dns1[] = "dns1";
        constexpr char dns2[] = "dns2";

        constexpr char serverIndex[] = "serverIndex";
        constexpr char description[] = "description";
        constexpr char name[] = "name";
        constexpr char cert[] = "cert";
        constexpr char cacert[] = "cacert";
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
        constexpr char mtu[] = "mtu";
        constexpr char allowed_ips[] = "allowed_ips";
        constexpr char persistent_keep_alive[] = "persistent_keep_alive";

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

        constexpr char junkPacketCount[] = "Jc";
        constexpr char junkPacketMinSize[] = "Jmin";
        constexpr char junkPacketMaxSize[] = "Jmax";
        constexpr char initPacketJunkSize[] = "S1";
        constexpr char responsePacketJunkSize[] = "S2";
        constexpr char initPacketMagicHeader[] = "H1";
        constexpr char responsePacketMagicHeader[] = "H2";
        constexpr char underloadPacketMagicHeader[] = "H3";
        constexpr char transportPacketMagicHeader[] = "H4";

        constexpr char openvpn[] = "openvpn";
        constexpr char wireguard[] = "wireguard";
        constexpr char shadowsocks[] = "shadowsocks";
        constexpr char cloak[] = "cloak";
        constexpr char sftp[] = "sftp";
        constexpr char awg[] = "awg";
        constexpr char xray[] = "xray";
        constexpr char ssxray[] = "ssxray";
        constexpr char socks5proxy[] = "socks5proxy";

        constexpr char configVersion[] = "config_version";

        constexpr char splitTunnelSites[] = "splitTunnelSites";
        constexpr char splitTunnelType[] = "splitTunnelType";

        constexpr char splitTunnelApps[] = "splitTunnelApps";
        constexpr char appSplitTunnelType[] = "appSplitTunnelType";

        constexpr char killSwitchOption[] = "killSwitchOption";

        constexpr char crc[] = "crc";

        constexpr char clientId[] = "clientId";

    }

    namespace protocols
    {

        namespace dns
        {
            constexpr char amneziaDnsIp[] = "172.29.172.254";
        }

        namespace openvpn
        {
            constexpr char defaultSubnetAddress[] = "10.8.0.0";
            constexpr char defaultSubnetMask[] = "255.255.255.0";
            constexpr char defaultSubnetCidr[] = "24";
            constexpr char defaultMtu[] = "1500";

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

        namespace shadowsocks
        {
            constexpr char ssKeyPath[] = "/opt/amnezia/shadowsocks/shadowsocks.key";
            constexpr char defaultPort[] = "6789";
            constexpr char defaultLocalProxyPort[] = "8585";
            constexpr char defaultCipher[] = "chacha20-ietf-poly1305";
        }

        namespace xray
        {
            constexpr char serverConfigPath[] = "/opt/amnezia/xray/server.json";
            constexpr char uuidPath[] = "/opt/amnezia/xray/xray_uuid.key";
            constexpr char PublicKeyPath[] = "/opt/amnezia/xray/xray_public.key";
            constexpr char PrivateKeyPath[] = "/opt/amnezia/xray/xray_private.key";
            constexpr char shortidPath[] = "/opt/amnezia/xray/xray_short_id.key";
            constexpr char defaultSite[] = "www.googletagmanager.com";

            constexpr char defaultPort[] = "443";
            constexpr char defaultLocalProxyPort[] = "10808";
            constexpr char defaultLocalAddr[] = "10.33.0.2";
        }

        namespace cloak
        {
            constexpr char ckPublicKeyPath[] = "/opt/amnezia/cloak/cloak_public.key";
            constexpr char ckBypassUidKeyPath[] = "/opt/amnezia/cloak/cloak_bypass_uid.key";
            constexpr char ckAdminKeyPath[] = "/opt/amnezia/cloak/cloak_admin_uid.key";
            constexpr char defaultPort[] = "443";
            constexpr char defaultRedirSite[] = "tile.openstreetmap.org";
            constexpr char defaultCipher[] = "chacha20-poly1305";
        }

        namespace wireguard
        {
            constexpr char defaultSubnetAddress[] = "10.8.1.0";
            constexpr char defaultSubnetMask[] = "255.255.255.0";
            constexpr char defaultSubnetCidr[] = "24";

            constexpr char defaultPort[] = "51820";

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
            constexpr char defaultMtu[] = "1280";
#else
            constexpr char defaultMtu[] = "1376";
#endif
            constexpr char serverConfigPath[] = "/opt/amnezia/wireguard/wg0.conf";
            constexpr char serverPublicKeyPath[] = "/opt/amnezia/wireguard/wireguard_server_public_key.key";
            constexpr char serverPskKeyPath[] = "/opt/amnezia/wireguard/wireguard_psk.key";

        }

        namespace sftp
        {
            constexpr char defaultUserName[] = "sftp_user";

        } // namespace sftp

        namespace awg
        {
            constexpr char defaultPort[] = "55424";
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
            constexpr char defaultMtu[] = "1280";
#else
            constexpr char defaultMtu[] = "1376";
#endif

            constexpr char serverConfigPath[] = "/opt/amnezia/awg/wg0.conf";
            constexpr char serverPublicKeyPath[] = "/opt/amnezia/awg/wireguard_server_public_key.key";
            constexpr char serverPskKeyPath[] = "/opt/amnezia/awg/wireguard_psk.key";

            constexpr char defaultJunkPacketCount[] = "3";
            constexpr char defaultJunkPacketMinSize[] = "10";
            constexpr char defaultJunkPacketMaxSize[] = "30";
            constexpr char defaultInitPacketJunkSize[] = "15";
            constexpr char defaultResponsePacketJunkSize[] = "18";
            constexpr char defaultInitPacketMagicHeader[] = "1020325451";
            constexpr char defaultResponsePacketMagicHeader[] = "3288052141";
            constexpr char defaultTransportPacketMagicHeader[] = "2528465083";
            constexpr char defaultUnderloadPacketMagicHeader[] = "1766607858";
        }

        namespace socks5Proxy
        {
            constexpr char defaultUserName[] = "proxy_user";
            constexpr char defaultPort[] = "38080";

            constexpr char proxyConfigPath[] = "/usr/local/3proxy/conf/3proxy.cfg";
        }

    } // namespace protocols

    namespace ProtocolEnumNS
    {
        Q_NAMESPACE

        enum TransportProto {
            Udp,
            Tcp,
            TcpAndUdp
        };
        Q_ENUM_NS(TransportProto)

        enum Proto {
            Any = 0,
            OpenVpn,
            ShadowSocks,
            Cloak,
            WireGuard,
            Awg,
            Ikev2,
            L2tp,
            Xray,
            SSXray,

            // non-vpn
            TorWebSite,
            Dns,
            Sftp,
            Socks5Proxy
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

        Q_INVOKABLE static int getPortForInstall(Proto p);

        Q_INVOKABLE static int defaultPort(Proto p);
        Q_INVOKABLE static bool defaultPortChangeable(Proto p);

        Q_INVOKABLE static TransportProto defaultTransportProto(Proto p);
        Q_INVOKABLE static bool defaultTransportProtoChangeable(Proto p);

        Q_INVOKABLE static QString key_proto_config_data(Proto p);
        Q_INVOKABLE static QString key_proto_config_path(Proto p);
    };
} // namespace amnezia

QDebug operator<<(QDebug debug, const amnezia::Proto &p);

#endif // PROTOCOLS_DEFS_H
