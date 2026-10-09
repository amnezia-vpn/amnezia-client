package org.amnezia.vpn.protocol.xray

import org.json.JSONArray
import org.json.JSONObject

internal fun replaceResolvedEndpointAddresses(
    xrayConfig: JSONObject,
    hostName: String,
    ipAddress: String,
) {
    val outbounds = xrayConfig.optJSONArray("outbounds") ?: return

    for (i in 0 until outbounds.length()) {
        val outbound = outbounds.optJSONObject(i) ?: continue
        val settings = outbound.optJSONObject("settings") ?: continue

        replaceAddressEntries(settings.optJSONArray("vnext"), hostName, ipAddress)
        replaceAddressEntries(settings.optJSONArray("servers"), hostName, ipAddress)
    }
}

private fun replaceAddressEntries(
    entries: JSONArray?,
    hostName: String,
    ipAddress: String,
) {
    if (entries == null) return

    for (i in 0 until entries.length()) {
        val entry = entries.optJSONObject(i) ?: continue
        if (entry.optString("address") == hostName) {
            entry.put("address", ipAddress)
        }
    }
}
