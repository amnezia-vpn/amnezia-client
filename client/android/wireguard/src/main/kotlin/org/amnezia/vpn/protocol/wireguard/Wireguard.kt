package org.amnezia.vpn.protocol.wireguard

import android.net.VpnService.Builder
import java.io.IOException
import java.util.Locale
import java.util.TreeMap
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext
import org.amnezia.awg.GoBackend
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.net.InetEndpoint
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.parseInetAddress
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
            val config = GoBackend.awgGetConfig(tunnelHandle) ?: return Statistics.EMPTY_STATISTICS
            return Statistics.build {
                var optsCount = 0
                config.splitToSequence("\n").forEach { line ->
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

    override fun internalInit() {
        if (!isInitialized) loadSharedLibrary(context, "wg-go")
    }

    override suspend fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        val wireguardConfig = parseConfig(config)
        val startTime = System.currentTimeMillis()
        start(wireguardConfig, vpnBuilder, protect)
        waitForConnection(startTime)
        state.value = CONNECTED
    }

    private suspend fun waitForConnection(startTime: Long) {
        Log.d(TAG, "Waiting for connection")
        withContext(Dispatchers.IO) {
            val time = String.format(Locale.ROOT,"%.3f", startTime / 1000.0)
            try {
                delay(1000)
                var log = getLogcat(time)
                Log.d(TAG, "First waiting log: $log")
                // check that there is a connection log,
                // to avoid infinite connection
                if (!log.contains("Attaching to interface")) {
                    Log.w(TAG, "Logs do not contain a connection log")
                    return@withContext
                }
                while (!log.contains("Received handshake response")) {
                    delay(1000)
                    log = getLogcat(time)
                }
            } catch (e: IOException) {
                Log.e(TAG, "Failed to get logcat: $e")
            }
        }
    }

    private fun getLogcat(time: String): String =
        ProcessBuilder("logcat", "--buffer=main", "--format=raw", "*:S AmneziaWG/awg0", "-t", time)
            .redirectErrorStream(true)
            .start()
            .inputStream.reader().readText()

    protected open fun parseConfig(config: JSONObject): WireguardConfig {
        val configDataJson = config.getJSONObject("wireguard_config_data")
        val configData = parseConfigData(configDataJson.getString("config"))
        return WireguardConfig.build {
            configWireguard(configData, configDataJson)
            configSplitTunneling(config)
            configAppSplitTunneling(config)
        }
    }

    protected fun WireguardConfig.Builder.configWireguard(configData: Map<String, String>, configDataJson: JSONObject) {
        configData["Address"]?.split(",")?.map { address ->
            InetNetwork.parse(address.trim())
        }?.forEach(::addAddress)

        configData["DNS"]?.split(",")?.map { dns ->
            parseInetAddress(dns.trim())
        }?.forEach(::addDnsServer)

        val defRoutes = hashSetOf(
            InetNetwork("0.0.0.0", 0),
            InetNetwork("::", 0)
        )
        val routes = hashSetOf<InetNetwork>()
        configData["AllowedIPs"]?.split(",")?.map { route ->
            InetNetwork.parse(route.trim())
        }?.forEach(routes::add)
        // if the allowed IPs list contains at least one non-default route, disable global split tunneling
        if (routes.any { it !in defRoutes }) disableSplitTunneling()
        addRoutes(routes)

        configDataJson.optString("mtu").let { mtu ->
            if (mtu.isNotEmpty()) {
                setMtu(mtu.toInt())
            } else {
                configData["MTU"]?.let { setMtu(it.toInt()) }
            }
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
            Log.i(TAG, "awg-go backend ${GoBackend.awgVersion()}")
            tunnelHandle = GoBackend.awgTurnOn(ifName, tunFd.detachFd(), config.toWgUserspaceString())
        }

        if (tunnelHandle < 0) {
            tunnelHandle = -1
            throw VpnStartException("Wireguard tunnel creation error")
        }

        if (!protect(GoBackend.awgGetSocketV4(tunnelHandle)) || !protect(GoBackend.awgGetSocketV6(tunnelHandle))) {
            GoBackend.awgTurnOff(tunnelHandle)
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
        GoBackend.awgTurnOff(handleToClose)
        state.value = DISCONNECTED
    }

    override fun reconnectVpn(vpnBuilder: Builder) {
        state.value = CONNECTED
    }
}
