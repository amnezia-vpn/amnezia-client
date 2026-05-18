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
        constexpr QLatin1String telemt("telemt");

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

        // ── Xray-specific keys ────────────────────────────────────────

        // Security
        constexpr QLatin1String xraySecurity("xray_security");       // none | tls | reality
        constexpr QLatin1String xrayFlow("xray_flow");               // "" | xtls-rprx-vision | xtls-rprx-vision-udp443
        constexpr QLatin1String xrayFingerprint("xray_fingerprint"); // Mozilla/5.0 | chrome | firefox | ...
        constexpr QLatin1String xraySni("xray_sni");                 // Server Name (SNI)
        constexpr QLatin1String xrayAlpn("xray_alpn");               // HTTP/2 | HTTP/1.1 | HTTP/2,HTTP/1.1

        // Transport — common
        constexpr QLatin1String xrayTransport("xray_transport"); // raw | xhttp | mkcp

        // Transport — XHTTP
        constexpr QLatin1String xhttpMode("xhttp_mode"); // Auto | Packet-up | Stream-up | Stream-one
        constexpr QLatin1String xhttpHost("xhttp_host");
        constexpr QLatin1String xhttpPath("xhttp_path");
        constexpr QLatin1String xhttpHeadersTemplate("xhttp_headers_template"); // HTTP | None
        constexpr QLatin1String xhttpUplinkMethod("xhttp_uplink_method");       // POST | PUT | PATCH
        constexpr QLatin1String xhttpDisableGrpc("xhttp_disable_grpc");         // bool
        constexpr QLatin1String xhttpDisableSse("xhttp_disable_sse");           // bool

        // Transport — XHTTP Session & Sequence
        constexpr QLatin1String xhttpSessionPlacement("xhttp_session_placement"); // Path | Header | Cookie | None
        constexpr QLatin1String xhttpSessionKey("xhttp_session_key");
        constexpr QLatin1String xhttpSeqPlacement("xhttp_seq_placement");
        constexpr QLatin1String xhttpSeqKey("xhttp_seq_key");
        constexpr QLatin1String xhttpUplinkDataPlacement("xhttp_uplink_data_placement"); // Body | Query
        constexpr QLatin1String xhttpUplinkDataKey("xhttp_uplink_data_key");

        // Transport — XHTTP Traffic Shaping
        constexpr QLatin1String xhttpUplinkChunkSize("xhttp_uplink_chunk_size");
        constexpr QLatin1String xhttpScMaxBufferedPosts("xhttp_sc_max_buffered_posts");
        constexpr QLatin1String xhttpScMaxEachPostBytesMin("xhttp_sc_max_each_post_bytes_min");
        constexpr QLatin1String xhttpScMaxEachPostBytesMax("xhttp_sc_max_each_post_bytes_max");
        constexpr QLatin1String xhttpScMinPostsIntervalMsMin("xhttp_sc_min_posts_interval_ms_min");
        constexpr QLatin1String xhttpScMinPostsIntervalMsMax("xhttp_sc_min_posts_interval_ms_max");
        constexpr QLatin1String xhttpScStreamUpServerSecsMin("xhttp_sc_stream_up_server_secs_min");
        constexpr QLatin1String xhttpScStreamUpServerSecsMax("xhttp_sc_stream_up_server_secs_max");

        // Transport — mKCP
        constexpr QLatin1String mkcpTti("mkcp_tti");
        constexpr QLatin1String mkcpUplinkCapacity("mkcp_uplink_capacity");
        constexpr QLatin1String mkcpDownlinkCapacity("mkcp_downlink_capacity");
        constexpr QLatin1String mkcpReadBufferSize("mkcp_read_buffer_size");
        constexpr QLatin1String mkcpWriteBufferSize("mkcp_write_buffer_size");
        constexpr QLatin1String mkcpCongestion("mkcp_congestion"); // bool

        // xPadding
        constexpr QLatin1String xPaddingBytesMin("xpadding_bytes_min");
        constexpr QLatin1String xPaddingBytesMax("xpadding_bytes_max");
        constexpr QLatin1String xPaddingObfsMode("xpadding_obfs_mode"); // bool
        constexpr QLatin1String xPaddingKey("xpadding_key");
        constexpr QLatin1String xPaddingHeader("xpadding_header");
        constexpr QLatin1String xPaddingPlacement("xpadding_placement"); // Cookie | Header | Query | Body
        constexpr QLatin1String xPaddingMethod("xpadding_method");       // Repeat-x | Random | Zero

        // xmux
        constexpr QLatin1String xmuxEnabled("xmux_enabled"); // bool
        constexpr QLatin1String xmuxMaxConcurrencyMin("xmux_max_concurrency_min");
        constexpr QLatin1String xmuxMaxConcurrencyMax("xmux_max_concurrency_max");
        constexpr QLatin1String xmuxMaxConnectionsMin("xmux_max_connections_min");
        constexpr QLatin1String xmuxMaxConnectionsMax("xmux_max_connections_max");
        constexpr QLatin1String xmuxCMaxReuseTimesMin("xmux_c_max_reuse_times_min");
        constexpr QLatin1String xmuxCMaxReuseTimesMax("xmux_c_max_reuse_times_max");
        constexpr QLatin1String xmuxHMaxRequestTimesMin("xmux_h_max_request_times_min");
        constexpr QLatin1String xmuxHMaxRequestTimesMax("xmux_h_max_request_times_max");
        constexpr QLatin1String xmuxHMaxReusableSecsMin("xmux_h_max_reusable_secs_min");
        constexpr QLatin1String xmuxHMaxReusableSecsMax("xmux_h_max_reusable_secs_max");
        constexpr QLatin1String xmuxHKeepAlivePeriod("xmux_h_keep_alive_period");
    }
}

#endif
