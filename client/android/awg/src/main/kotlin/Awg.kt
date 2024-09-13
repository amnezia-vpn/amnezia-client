package org.amnezia.vpn.protocol.awg

import org.amnezia.vpn.protocol.wireguard.Wireguard
import org.amnezia.vpn.util.optStringOrNull
import org.json.JSONObject

class Awg : Wireguard() {

    override val ifName: String = "awg0"

    override fun parseConfig(config: JSONObject): AwgConfig {
        val configData = config.getJSONObject("awg_config_data")
        return AwgConfig.build {
            configWireguard(config, configData)
            configSplitTunneling(config)
            configAppSplitTunneling(config)
            configData.optStringOrNull("Jc")?.let { setJc(it.toInt()) }
            configData.optStringOrNull("Jmin")?.let { setJmin(it.toInt()) }
            configData.optStringOrNull("Jmax")?.let { setJmax(it.toInt()) }
            configData.optStringOrNull("S1")?.let { setS1(it.toInt()) }
            configData.optStringOrNull("S2")?.let { setS2(it.toInt()) }
            configData.optStringOrNull("H1")?.let { setH1(it.toLong()) }
            configData.optStringOrNull("H2")?.let { setH2(it.toLong()) }
            configData.optStringOrNull("H3")?.let { setH3(it.toLong()) }
            configData.optStringOrNull("H4")?.let { setH4(it.toLong()) }
        }
    }
}
