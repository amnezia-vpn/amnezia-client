package com.fblink.vpn.protocol.awg

import com.fblink.vpn.protocol.wireguard.Wireguard
import com.fblink.vpn.protocol.wireguard.WireguardConfig
import org.json.JSONObject

class Awg : Wireguard() {

    override val ifName: String = "awg0"

    override fun parseConfig(config: JSONObject): WireguardConfig {
        val configData = config.getJSONObject("awg_config_data")
        return WireguardConfig.build {
            setUseProtocolExtension(true)
            configExtensionParameters(configData)
            configWireguard(config, configData)
            configSplitTunneling(config)
            configAppSplitTunneling(config)
        }
    }
}
