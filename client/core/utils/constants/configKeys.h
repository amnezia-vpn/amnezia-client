#ifndef CONFIGKEYS_H
#define CONFIGKEYS_H

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
        constexpr char isObfuscationEnabled[] = "isObfuscationEnabled";

        constexpr char junkPacketCount[] = "Jc";
        constexpr char junkPacketMinSize[] = "Jmin";
        constexpr char junkPacketMaxSize[] = "Jmax";
        constexpr char initPacketJunkSize[] = "S1";
        constexpr char responsePacketJunkSize[] = "S2";
        constexpr char cookieReplyPacketJunkSize[] = "S3";
        constexpr char transportPacketJunkSize[] = "S4";
        constexpr char initPacketMagicHeader[] = "H1";
        constexpr char responsePacketMagicHeader[] = "H2";
        constexpr char underloadPacketMagicHeader[] = "H3";
        constexpr char transportPacketMagicHeader[] = "H4";
        constexpr char specialJunk1[] = "I1";
        constexpr char specialJunk2[] = "I2";
        constexpr char specialJunk3[] = "I3";
        constexpr char specialJunk4[] = "I4";
        constexpr char specialJunk5[] = "I5";

        constexpr char protocolVersion[] = "protocol_version";

        constexpr char openvpn[] = "openvpn";
        constexpr char wireguard[] = "wireguard";
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

        constexpr char allowedDnsServers[] = "allowedDnsServers";

        constexpr char killSwitchOption[] = "killSwitchOption";

        constexpr char crc[] = "crc";

        constexpr char clientId[] = "clientId";

        constexpr char nameOverriddenByUser[] = "nameOverriddenByUser";

    }
}

#endif // CONFIGKEYS_H


