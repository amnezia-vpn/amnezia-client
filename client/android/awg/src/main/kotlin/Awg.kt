package org.amnezia.vpn.protocol.awg

import org.amnezia.vpn.protocol.wireguard.Wireguard
import org.json.JSONObject

/**
 *    Config example:
 *    {
 *        "protocol": "awg",
 *        "description": "Server 1",
 *        "dns1": "1.1.1.1",
 *        "dns2": "1.0.0.1",
 *        "hostName": "100.100.100.0",
 *        "splitTunnelSites": [
 *        ],
 *        "splitTunnelType": 0,
 *        "awg_config_data": {
 *            "H1": "969537490",
 *            "H2": "481688153",
 *            "H3": "2049399200",
 *            "H4": "52029755",
 *            "Jc": "3",
 *            "Jmax": "1000",
 *            "Jmin": "50",
 *            "S1": "49",
 *            "S2": "60",
 *            "client_ip": "10.8.1.1",
 *            "hostName": "100.100.100.0",
 *            "port": 12345,
 *            "client_pub_key": "clientPublicKeyBase64",
 *            "client_priv_key": "privateKeyBase64",
 *            "psk_key": "presharedKeyBase64",
 *            "server_pub_key": "publicKeyBase64",
 *            "config": "[Interface]
 *                       Address = 10.8.1.1/32
 *                       DNS = 1.1.1.1, 1.0.0.1
 *                       PrivateKey = privateKeyBase64
 *                       Jc = 3
 *                       Jmin = 50
 *                       Jmax = 1000
 *                       S1 = 49
 *                       S2 = 60
 *                       H1 = 969537490
 *                       H2 = 481688153
 *                       H3 = 2049399200
 *                       H4 = 52029755
 *
 *                       [Peer]
 *                       PublicKey = publicKeyBase64
 *                       PresharedKey = presharedKeyBase64
 *                       AllowedIPs = 0.0.0.0/0, ::/0
 *                       Endpoint = 100.100.100.0:12345
 *                       PersistentKeepalive = 25
 *                       "
 *        }
 *    }
 */

class Awg : Wireguard() {

    override val ifName: String = "awg0"

    override fun parseConfig(config: JSONObject): AwgConfig {
        val configDataJson = config.getJSONObject("awg_config_data")
        val configData = parseConfigData(configDataJson.getString("config"))
        return AwgConfig.build {
            configureWireguard(wireguardConfigBuilder(configData))
            configData["Jc"]?.let { setJc(it.toInt()) }
            configData["Jmin"]?.let { setJmin(it.toInt()) }
            configData["Jmax"]?.let { setJmax(it.toInt()) }
            configData["S1"]?.let { setS1(it.toInt()) }
            configData["S2"]?.let { setS2(it.toInt()) }
            configData["H1"]?.let { setH1(it.toLong()) }
            configData["H2"]?.let { setH2(it.toLong()) }
            configData["H3"]?.let { setH3(it.toLong()) }
            configData["H4"]?.let { setH4(it.toLong()) }
        }
    }
}
