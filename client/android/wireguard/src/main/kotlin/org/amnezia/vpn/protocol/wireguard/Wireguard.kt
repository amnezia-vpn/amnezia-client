package org.amnezia.vpn.protocol.wireguard

import android.content.Context
import android.net.VpnService.Builder
import java.util.TreeMap
import com.wireguard.android.backend.GoBackend
import org.amnezia.vpn.Log
import org.amnezia.vpn.protocol.InetEndpoint
import org.amnezia.vpn.protocol.InetNetwork
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VPN_SESSION_NAME
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.protocol.parseInetAddress
import org.json.JSONObject

private const val TAG = "Wireguard"

class Wireguard(context: Context) : Protocol(context) {

    private var tunnelHandle: Int = -1
    private lateinit var wireguardConfig: WireguardConfig

    override val statistics: Statistics
        get() {
            if (tunnelHandle == -1) return Statistics.EMPTY_STATISTICS
            val config = GoBackend.wgGetConfig(tunnelHandle) ?: return Statistics.EMPTY_STATISTICS
            return Statistics.build {
                var optsCount = 0
                config.splitToSequence("\\n").forEach { line ->
                    with(line) {
                        when {
                            startsWith("rx_bytes=") -> setRxBytes(substring(9).toLong()).also { ++optsCount }
                            startsWith("tx_bytes=") -> setTxBytes(substring(9).toLong()).also { ++optsCount }
                            else -> {}
                        }
                    }
                    if (optsCount == 2) return@forEach
                }
            }
        }

    override fun initialize() {
        loadSharedLibrary(context, "wg-go")
    }

    override fun parseConfig(config: JSONObject) {
        val configDataJson = config.getJSONObject("wireguard_config_data")
        val configData = parseConfigData(configDataJson.getString("config"))
        wireguardConfig = WireguardConfig.build {
            configureBaseProtocol(true) {
                configData["Address"]?.let { addAddress(InetNetwork.parse(it)) }
                configData["DNS"]?.split(",")?.map { dns ->
                    parseInetAddress(dns.trim())
                }?.forEach(::addDnsServer)
                configData["AllowedIPs"]?.split(",")?.map { route ->
                    InetNetwork.parse(route.trim())
                }?.forEach(::addRoute)
                setMtu(configData["MTU"]?.toInt() ?: WIREGUARD_DEFAULT_MTU)
            }
            configData["Endpoint"]?.let { setEndpoint(InetEndpoint.parse(it)) }
            configData["PersistentKeepalive"]?.let { setPersistentKeepalive(it.toInt()) }
            configData["PrivateKey"]?.let { setPrivateKeyHex(it.base64ToHex()) }
            configData["PublicKey"]?.let { setPublicKeyHex(it.base64ToHex()) }
            configData["PresharedKey"]?.let { setPreSharedKeyHex(it.base64ToHex()) }
        }
        this.config = wireguardConfig.baseProtocolConfig
    }

    private fun parseConfigData(data: String): Map<String, String> {
        val parsedData = TreeMap<String, String>(String.CASE_INSENSITIVE_ORDER)
        data.lineSequence()
            .filter { it.isNotEmpty() && !it.startsWith('[') }
            .forEach { line ->
                val attr = line.split("=", limit = 2)
                parsedData[attr.first().trim()] = attr.last().trim()
            }
        return parsedData
    }

    override fun startVpn(vpnBuilder: Builder, protect: (Int) -> Boolean) {
        if (tunnelHandle != -1) {
            Log.w(TAG, "Tunnel already up")
            return
        }

        buildVpnInterface(vpnBuilder)

        vpnBuilder.establish().use { tunFd ->
            if (tunFd == null) {
                throw VpnStartException("Create VPN interface: permission not granted or revoked")
            }
            Log.v(TAG, "Wg-go backend ${GoBackend.wgVersion()}")
            tunnelHandle = GoBackend.wgTurnOn(VPN_SESSION_NAME, tunFd.detachFd(), wireguardConfig.toWgUserspaceString())
        }

        if (tunnelHandle < 0) {
            tunnelHandle = -1
            throw VpnStartException("Wireguard tunnel creation error")
        }

        if (!protect(GoBackend.wgGetSocketV4(tunnelHandle)) || !protect(GoBackend.wgGetSocketV6(tunnelHandle))) {
            GoBackend.wgTurnOff(tunnelHandle)
            tunnelHandle = -1
            throw VpnStartException("Protect VPN interface: permission not granted or revoked")
        }
    }

    override fun stopVpn() {
        if (tunnelHandle == -1) {
            Log.w(TAG, "Tunnel already down")
            return
        }
        val handleToClose = tunnelHandle
        tunnelHandle = -1
        GoBackend.wgTurnOff(handleToClose)
    }
}
