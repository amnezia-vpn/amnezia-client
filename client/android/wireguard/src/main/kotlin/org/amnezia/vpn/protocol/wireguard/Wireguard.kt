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
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.protocol.parseInetAddress
import org.json.JSONObject

/**
 *    Config example:
 *    {
 *        "protocol": "wireguard",
 *        "description": "Server 1",
 *        "dns1": "1.1.1.1",
 *        "dns2": "1.0.0.1",
 *        "hostName": "100.100.100.0",
 *        "splitTunnelSites": [
 *        ],
 *        "splitTunnelType": 0,
 *        "wireguard_config_data": {
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

private const val TAG = "Wireguard"

open class Wireguard : Protocol() {

    private var tunnelHandle: Int = -1
    protected open val ifName: String = "amn0"

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

    override fun initialize(context: Context) {
        loadSharedLibrary(context, "wg-go")
    }

    override fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        val wireguardConfig = parseConfig(config)
        start(wireguardConfig, vpnBuilder, protect)
    }

    protected open fun parseConfig(config: JSONObject): WireguardConfig {
        val configDataJson = config.getJSONObject("wireguard_config_data")
        val configData = parseConfigData(configDataJson.getString("config"))
        return WireguardConfig.build(wireguardConfigBuilder(configData))
    }

    protected fun wireguardConfigBuilder(configData: Map<String, String>): WireguardConfig.Builder.() -> Unit =
        {
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

    protected fun parseConfigData(data: String): Map<String, String> {
        val parsedData = TreeMap<String, String>(String.CASE_INSENSITIVE_ORDER)
        data.lineSequence()
            .filter { it.isNotEmpty() && !it.startsWith('[') }
            .forEach { line ->
                val attr = line.split("=", limit = 2)
                parsedData[attr.first().trim()] = attr.last().trim()
            }
        return parsedData
    }

    private fun start(config: WireguardConfig, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        if (tunnelHandle != -1) {
            Log.w(TAG, "Tunnel already up")
            return
        }

        buildVpnInterface(config, vpnBuilder)

        vpnBuilder.establish().use { tunFd ->
            if (tunFd == null) {
                throw VpnStartException("Create VPN interface: permission not granted or revoked")
            }
            Log.v(TAG, "Wg-go backend ${GoBackend.wgVersion()}")
            tunnelHandle = GoBackend.wgTurnOn(ifName, tunFd.detachFd(), config.toWgUserspaceString())
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
