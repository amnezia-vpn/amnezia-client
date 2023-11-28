package org.amnezia.vpn.protocol.cloak

import android.util.Base64
import net.openvpn.ovpn3.ClientAPI_Config
import org.amnezia.vpn.protocol.openvpn.OpenVpn
import org.json.JSONObject

/**
 *    Config Example:
 *    {
 *     "protocol": "cloak",
 *     "description": "Server 1",
 *     "dns1": "1.1.1.1",
 *     "dns2": "1.0.0.1",
 *     "hostName": "100.100.100.0",
 *     "splitTunnelSites": [
 *     ],
 *     "splitTunnelType": 0,
 *     "openvpn_config_data": {
 *           "config": "openVpnConfig"
 *     }
 *     "cloak_config_data": {
 *          "BrowserSig": "chrome",
 *          "EncryptionMethod": "aes-gcm",
 *          "NumConn": 1,
 *          "ProxyMethod": "openvpn",
 *          "PublicKey": "PublicKey=",
 *          "RemoteHost": "100.100.100.0",
 *          "RemotePort": "443",
 *          "ServerName": "servername",
 *          "StreamTimeout": 300,
 *          "Transport": "direct",
 *          "UID": "UID="
 *     }
 * }
 */

class Cloak : OpenVpn() {

    override fun parseConfig(config: JSONObject): ClientAPI_Config {
        val openVpnConfig = ClientAPI_Config()

        val openVpnConfigStr = config.getJSONObject("openvpn_config_data").getString("config")
        val cloakConfigJson = checkCloakJson(config.getJSONObject("cloak_config_data"))
        val cloakConfigStr = Base64.encodeToString(cloakConfigJson.toString().toByteArray(), Base64.DEFAULT)

        val configStr = "$openVpnConfigStr\n<cloak>\n$cloakConfigStr\n</cloak>\n"

        openVpnConfig.usePluggableTransports = true
        openVpnConfig.content = configStr
        return openVpnConfig
    }

    private fun checkCloakJson(cloakConfigJson: JSONObject): JSONObject {
        // todo: strange method
        if (!cloakConfigJson.has("NumConn")) cloakConfigJson.put("NumConn", 1)
        if (!cloakConfigJson.has("ProxyMethod")) cloakConfigJson.put("ProxyMethod", "openvpn")
        if (cloakConfigJson.has("port")) {
            val port = cloakConfigJson["port"]
            cloakConfigJson.remove("port")
            cloakConfigJson.put("RemotePort", port)
        }
        if (cloakConfigJson.has("remote")) {
            val remote = cloakConfigJson["remote"]
            cloakConfigJson.remove("remote")
            cloakConfigJson.put("RemoteHost", remote)
        }
        return cloakConfigJson
    }
}
