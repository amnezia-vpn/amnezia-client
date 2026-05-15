#ifndef CONFIGKEYS_H
#define CONFIGKEYS_H

#include <QLatin1String>

namespace amnezia
{
    namespace configKey
    {
        constexpr QLatin1String hostName("hostName");
        constexpr QLatin1String userName("userName");
        constexpr QLatin1String password("password");
        constexpr QLatin1String port("port");
        constexpr QLatin1String localPort("local_port");

        constexpr QLatin1String dns1("dns1");
        constexpr QLatin1String dns2("dns2");

        constexpr QLatin1String serverIndex("serverIndex");
        constexpr QLatin1String description("description");
        constexpr QLatin1String displayName("displayName");
        constexpr QLatin1String name("name");
        constexpr QLatin1String cert("cert");
        constexpr QLatin1String accessToken("api_key");
        constexpr QLatin1String config("config");
        constexpr QLatin1String configVersion("config_version");

        constexpr QLatin1String containers("containers");
        constexpr QLatin1String container("container");
        constexpr QLatin1String defaultContainer("defaultContainer");

        constexpr QLatin1String vpnProto("protocol");
        constexpr QLatin1String protocol("protocol");
        constexpr QLatin1String protocols("protocols");

        constexpr QLatin1String remote("remote");
        constexpr QLatin1String transportProto("transport_proto");
        constexpr QLatin1String cipher("cipher");
        constexpr QLatin1String hash("hash");
        constexpr QLatin1String ncpDisable("ncp_disable");
        constexpr QLatin1String tlsAuth("tls_auth");

        constexpr QLatin1String clientPrivKey("client_priv_key");
        constexpr QLatin1String clientPubKey("client_pub_key");
        constexpr QLatin1String serverPrivKey("server_priv_key");
        constexpr QLatin1String serverPubKey("server_pub_key");
        constexpr QLatin1String pskKey("psk_key");
        constexpr QLatin1String mtu("mtu");
        constexpr QLatin1String allowedIps("allowed_ips");
        constexpr QLatin1String persistentKeepAlive("persistent_keep_alive");

        constexpr QLatin1String clientIp("client_ip");

        constexpr QLatin1String site("site");
        constexpr QLatin1String blockOutsideDns("block_outside_dns");

        constexpr QLatin1String subnetAddress("subnet_address");
        constexpr QLatin1String subnetMask("subnet_mask");
        constexpr QLatin1String subnetCidr("subnet_cidr");

        constexpr QLatin1String additionalClientConfig("additional_client_config");
        constexpr QLatin1String additionalServerConfig("additional_server_config");

        constexpr QLatin1String lastConfig("last_config");

        constexpr QLatin1String isThirdPartyConfig("isThirdPartyConfig");
        constexpr QLatin1String isObfuscationEnabled("isObfuscationEnabled");

        constexpr QLatin1String junkPacketCount("Jc");
        constexpr QLatin1String junkPacketMinSize("Jmin");
        constexpr QLatin1String junkPacketMaxSize("Jmax");
        constexpr QLatin1String initPacketJunkSize("S1");
        constexpr QLatin1String responsePacketJunkSize("S2");
        constexpr QLatin1String cookieReplyPacketJunkSize("S3");
        constexpr QLatin1String transportPacketJunkSize("S4");
        constexpr QLatin1String initPacketMagicHeader("H1");
        constexpr QLatin1String responsePacketMagicHeader("H2");
        constexpr QLatin1String underloadPacketMagicHeader("H3");
        constexpr QLatin1String transportPacketMagicHeader("H4");
        constexpr QLatin1String specialJunk1("I1");
        constexpr QLatin1String specialJunk2("I2");
        constexpr QLatin1String specialJunk3("I3");
        constexpr QLatin1String specialJunk4("I4");
        constexpr QLatin1String specialJunk5("I5");

        constexpr QLatin1String protocolVersion("protocol_version");

        constexpr QLatin1String openvpn("openvpn");
        constexpr QLatin1String wireguard("wireguard");
        constexpr QLatin1String sftp("sftp");
        constexpr QLatin1String awg("awg");
        constexpr QLatin1String vless("vless");
        constexpr QLatin1String xray("xray");
        constexpr QLatin1String ssxray("ssxray");
        constexpr QLatin1String socks5proxy("socks5proxy");
        constexpr QLatin1String mtproxy("mtproxy");

        constexpr QLatin1String splitTunnelSites("splitTunnelSites");
        constexpr QLatin1String splitTunnelType("splitTunnelType");

        constexpr QLatin1String splitTunnelApps("splitTunnelApps");
        constexpr QLatin1String appSplitTunnelType("appSplitTunnelType");

        constexpr QLatin1String allowedDnsServers("allowedDnsServers");

        constexpr QLatin1String killSwitchOption("killSwitchOption");

        constexpr QLatin1String crc("crc");

        constexpr QLatin1String clientId("clientId");

        constexpr QLatin1String nameOverriddenByUser("nameOverriddenByUser");

        constexpr QLatin1String amneziaOpenvpn("amnezia-openvpn");
        constexpr QLatin1String amneziaWireguard("amnezia-wireguard");
        constexpr QLatin1String amneziaAwg("amnezia-awg");
        constexpr QLatin1String amneziaXray("amnezia-xray");
        constexpr QLatin1String amneziaSsxray("amnezia-ssxray");

        constexpr QLatin1String clientName("clientName");
        constexpr QLatin1String userData("userData");
        constexpr QLatin1String creationDate("creationDate");
        constexpr QLatin1String latestHandshake("latestHandshake");
        constexpr QLatin1String dataReceived("dataReceived");
        constexpr QLatin1String dataSent("dataSent");

        constexpr QLatin1String storageServerId("storageServerId");
    }
}

#endif
