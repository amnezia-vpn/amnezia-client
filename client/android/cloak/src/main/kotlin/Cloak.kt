package org.amnezia.vpn.protocol.cloak

import android.util.Base64
import net.openvpn.ovpn3.ClientAPI_Config
import org.amnezia.vpn.protocol.openvpn.OpenVpn
import org.json.JSONObject

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
        cloakConfigJson.put("NumConn", 1)
        cloakConfigJson.put("ProxyMethod", "openvpn")
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
