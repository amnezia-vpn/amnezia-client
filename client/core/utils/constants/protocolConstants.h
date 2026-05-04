#ifndef PROTOCOLCONSTANTS_H
#define PROTOCOLCONSTANTS_H

namespace amnezia
{

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

            constexpr char outbounds[] = "outbounds";
            constexpr char inbounds[] = "inbounds";
            constexpr char settings[] = "settings";
            constexpr char streamSettings[] = "streamSettings";
            constexpr char vnext[] = "vnext";
            constexpr char users[] = "users";
            constexpr char servers[] = "servers";
            constexpr char clients[] = "clients";
            constexpr char id[] = "id";
            constexpr char port[] = "port";
            constexpr char address[] = "address";
            constexpr char flow[] = "flow";
            constexpr char encryption[] = "encryption";
            constexpr char network[] = "network";
            constexpr char security[] = "security";
            constexpr char realitySettings[] = "realitySettings";
            constexpr char serverNames[] = "serverNames";
            constexpr char serverName[] = "serverName";
            constexpr char publicKey[] = "publicKey";
            constexpr char shortId[] = "shortId";
            constexpr char fingerprint[] = "fingerprint";
            constexpr char spiderX[] = "spiderX";
            constexpr char user[] = "user";
            constexpr char pass[] = "pass";
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
            // Config file keys ([Interface] / [Peer] sections) - case-sensitive
            constexpr char PrivateKey[] = "PrivateKey";
            constexpr char Address[] = "Address";
            constexpr char PublicKey[] = "PublicKey";
            constexpr char PresharedKey[] = "PresharedKey";
            constexpr char PreSharedKey[] = "PreSharedKey";
            constexpr char AllowedIPs[] = "AllowedIPs";
            constexpr char Endpoint[] = "Endpoint";
            constexpr char PersistentKeepalive[] = "PersistentKeepalive";
            constexpr char MTU[] = "MTU";

            constexpr char defaultSubnetAddress[] = "10.8.1.0";
            constexpr char defaultSubnetMask[] = "255.255.255.0";
            constexpr char defaultSubnetCidr[] = "24";

            constexpr char defaultPort[] = "51820";

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
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
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
            constexpr char defaultMtu[] = "1280";
#else
            constexpr char defaultMtu[] = "1376";
#endif

            constexpr char serverConfigPath[] = "/opt/amnezia/awg/awg0.conf";
            constexpr char serverLegacyConfigPath[] = "/opt/amnezia/awg/wg0.conf";
            constexpr char serverPublicKeyPath[] = "/opt/amnezia/awg/wireguard_server_public_key.key";
            constexpr char serverPskKeyPath[] = "/opt/amnezia/awg/wireguard_psk.key";

            constexpr char defaultJunkPacketCount[] = "3";
            constexpr char defaultJunkPacketMinSize[] = "10";
            constexpr char defaultJunkPacketMaxSize[] = "30";
            constexpr char defaultInitPacketJunkSize[] = "15";
            constexpr char defaultResponsePacketJunkSize[] = "18";
            constexpr char defaultCookieReplyPacketJunkSize[] = "20";
            constexpr char defaultTransportPacketJunkSize[] = "23";

            constexpr char defaultInitPacketMagicHeader[] = "1020325451";
            constexpr char defaultResponsePacketMagicHeader[] = "3288052141";
            constexpr char defaultTransportPacketMagicHeader[] = "2528465083";
            constexpr char defaultUnderloadPacketMagicHeader[] = "1766607858";
            constexpr char defaultSpecialJunk1[] = "<r 2><b 0x858000010001000000000669636c6f756403636f6d0000010001c00c000100010000105a00044d583737>";
            constexpr char defaultSpecialJunk2[] = "";
            constexpr char defaultSpecialJunk3[] = "";
            constexpr char defaultSpecialJunk4[] = "";
            constexpr char defaultSpecialJunk5[] = "";

            constexpr char awgV1_5[] = "1.5";
            constexpr char awgV2[] = "2";
        }

        namespace socks5Proxy
        {
            constexpr char defaultUserName[] = "proxy_user";
            constexpr char defaultPort[] = "38080";

            constexpr char proxyConfigPath[] = "/usr/local/3proxy/conf/3proxy.cfg";
        }

        namespace mtProxy
        {
            constexpr char secretKey[]            = "mtproxy_secret";
            constexpr char tagKey[]               = "mtproxy_tag";
            constexpr char tgLinkKey[]            = "mtproxy_tg_link";
            constexpr char tmeLinkKey[]           = "mtproxy_tme_link";
            constexpr char isEnabledKey[]         = "mtproxy_is_enabled";
            constexpr char publicHostKey[]        = "mtproxy_public_host";
            constexpr char transportModeKey[]     = "mtproxy_transport_mode";
            constexpr char tlsDomainKey[]         = "mtproxy_tls_domain";
            constexpr char additionalSecretsKey[] = "mtproxy_additional_secrets";
            constexpr char workersKey[]           = "mtproxy_workers";
            constexpr char workersModeKey[]       = "mtproxy_workers_mode";
            constexpr char natEnabledKey[]        = "mtproxy_nat_enabled";
            constexpr char natInternalIpKey[]     = "mtproxy_nat_internal_ip";
            constexpr char natExternalIpKey[]     = "mtproxy_nat_external_ip";

            constexpr char transportModeStandard[] = "standard";
            constexpr char transportModeFakeTLS[]  = "faketls";

            constexpr char workersModeAuto[]       = "auto";
            constexpr char workersModeManual[]     = "manual";

            constexpr char defaultPort[]           = "443";
            constexpr char defaultWorkers[]        = "2";
            constexpr int  maxWorkers              = 32;
            constexpr int  botTagHexLength         = 32;
            constexpr char defaultTlsDomain[]      = "googletagmanager.com";
        }

    } // namespace protocols
}

#endif // PROTOCOLCONSTANTS_H
