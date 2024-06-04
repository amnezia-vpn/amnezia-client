package org.amnezia.vpn.protocol.xray

import android.content.Context
import android.net.VpnService.Builder
import java.io.File
import java.io.IOException
import go.Seq
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.protocol.xray.libXray.DialerController
import org.amnezia.vpn.protocol.xray.libXray.LibXray
import org.amnezia.vpn.protocol.xray.libXray.Logger
import org.amnezia.vpn.protocol.xray.libXray.Tun2SocksConfig
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.parseInetAddress
import org.json.JSONObject

/**
 *    Config example:
 * {
 *     "appSplitTunnelType": 0,
 *     "config_version": 0,
 *     "description": "Server 1",
 *     "dns1": "1.1.1.1",
 *     "dns2": "1.0.0.1",
 *     "hostName": "100.100.100.0",
 *     "protocol": "xray",
 *     "splitTunnelApps": [],
 *     "splitTunnelSites": [],
 *     "splitTunnelType": 0,
 *     "xray_config_data": {
 *         "inbounds": [
 *             {
 *                 "listen": "127.0.0.1",
 *                 "port": 8080,
 *                 "protocol": "socks",
 *                 "settings": {
 *                     "udp": true
 *                 }
 *             }
 *         ],
 *         "log": {
 *             "loglevel": "error"
 *         },
 *         "outbounds": [
 *             {
 *                 "protocol": "vless",
 *                 "settings": {
 *                     "vnext": [
 *                         {
 *                             "address": "100.100.100.0",
 *                             "port": 443,
 *                             "users": [
 *                                 {
 *                                     "encryption": "none",
 *                                     "flow": "xtls-rprx-vision",
 *                                     "id": "id"
 *                                 }
 *                             ]
 *                         }
 *                     ]
 *                 },
 *                 "streamSettings": {
 *                     "network": "tcp",
 *                     "realitySettings": {
 *                         "fingerprint": "chrome",
 *                         "publicKey": "publicKey",
 *                         "serverName": "google.com",
 *                         "shortId": "id",
 *                         "spiderX": ""
 *                     },
 *                     "security": "reality"
 *                 }
 *             }
 *         ]
 *     }
 * }
 *
 */

private const val TAG = "Xray"
private const val LIBXRAY_TAG = "libXray"

class Xray : Protocol() {

    private var isRunning: Boolean = false
    override val statistics: Statistics = Statistics.EMPTY_STATISTICS

    override fun internalInit() {
        Seq.setContext(context)
        if (!isInitialized) {
            LibXray.initLogger(object : Logger {
                override fun warning(s: String) = Log.w(LIBXRAY_TAG, s)

                override fun error(s: String) = Log.e(LIBXRAY_TAG, s)

                override fun write(msg: ByteArray): Long {
                    Log.w(LIBXRAY_TAG, String(msg))
                    return msg.size.toLong()
                }
            }).isNotNullOrBlank { err ->
                Log.w(TAG, "Failed to initialize logger: $err")
            }
        }
    }

    override fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        if (isRunning) {
            Log.w(TAG, "XRay already running")
            return
        }

        val xrayJsonConfig = config.getJSONObject("xray_config_data")
        val xrayConfig = parseConfig(config, xrayJsonConfig)

        // for debug
        // xrayJsonConfig.getJSONObject("log").put("loglevel", "debug")
        xrayJsonConfig.getJSONObject("log").put("loglevel", "warning")
        // disable access log
        xrayJsonConfig.getJSONObject("log").put("access", "none")

        // replace socks address
        // (xrayJsonConfig.getJSONArray("inbounds")[0] as JSONObject).put("listen", "::1")

        start(xrayConfig, xrayJsonConfig.toString(), vpnBuilder, protect)
        state.value = CONNECTED
        isRunning = true
    }

    private fun parseConfig(config: JSONObject, xrayJsonConfig: JSONObject): XrayConfig {
        return XrayConfig.build {
            addAddress(XrayConfig.DEFAULT_IPV4_ADDRESS)

            config.optString("dns1").let {
                if (it.isNotBlank()) addDnsServer(parseInetAddress(it))
            }

            config.optString("dns2").let {
                if (it.isNotBlank()) addDnsServer(parseInetAddress(it))
            }

            addRoute(InetNetwork("0.0.0.0", 0))
            addRoute(InetNetwork("2000::0", 3))
            config.getString("hostName").let {
                excludeRoute(InetNetwork(it, 32))
            }

            config.optString("mtu").let {
                if (it.isNotBlank()) setMtu(it.toInt())
            }

            val socksConfig = xrayJsonConfig.getJSONArray("inbounds")[0] as JSONObject
            socksConfig.getInt("port").let { setSocksPort(it) }

            configSplitTunneling(config)
            configAppSplitTunneling(config)
        }
    }

    private fun start(config: XrayConfig, configJson: String, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        buildVpnInterface(config, vpnBuilder)

        DialerController { protect(it.toInt()) }.also {
            LibXray.registerDialerController(it).isNotNullOrBlank { err ->
                throw VpnStartException("Failed to register dialer controller: $err")
            }
            LibXray.registerListenerController(it).isNotNullOrBlank { err ->
                throw VpnStartException("Failed to register listener controller: $err")
            }
        }

        vpnBuilder.establish().use { tunFd ->
            if (tunFd == null) {
                throw VpnStartException("Create VPN interface: permission not granted or revoked")
            }
            Log.d(TAG, "Run tun2Socks")
            runTun2Socks(config, tunFd.detachFd())

            Log.d(TAG, "Run XRay")
            Log.i(TAG, "xray ${LibXray.xrayVersion()}")
            LibXray.initXray()
            val geoDir = File(context.getDir("assets", Context.MODE_PRIVATE), "geo").absolutePath
            val configPath = File(context.cacheDir, "config.json")
            Log.d(TAG, "xray.location.asset: $geoDir")
            Log.d(TAG, "config: $configPath")
            try {
                configPath.writeText(configJson)
            } catch (e: IOException) {
                LibXray.stopTun2Socks()
                throw VpnStartException("Failed to write xray config: ${e.message}")
            }
            LibXray.runXray(geoDir, configPath.absolutePath, config.maxMemory).isNotNullOrBlank { err ->
                LibXray.stopTun2Socks()
                throw VpnStartException("Failed to start xray: $err")
            }
        }
    }

    override fun stopVpn() {
        LibXray.stopXray().isNotNullOrBlank { err ->
            Log.e(TAG, "Failed to stop XRay: $err")
        }
        LibXray.stopTun2Socks().isNotNullOrBlank { err ->
            Log.e(TAG, "Failed to stop tun2Socks: $err")
        }

        isRunning = false
        state.value = DISCONNECTED
    }

    override fun reconnectVpn(vpnBuilder: Builder) {
        state.value = CONNECTED
    }

    private fun runTun2Socks(config: XrayConfig, fd: Int) {
        val tun2SocksConfig = Tun2SocksConfig().apply {
            mtu = config.mtu.toLong()
            proxy = "socks5://127.0.0.1:${config.socksPort}"
            device = "fd://$fd"
            logLevel = "warning"
        }
        LibXray.startTun2Socks(tun2SocksConfig, fd.toLong()).isNotNullOrBlank { err ->
            throw VpnStartException("Failed to start tun2socks: $err")
        }
    }
}

private fun String?.isNotNullOrBlank(block: (String) -> Unit) {
    if (!this.isNullOrBlank()) {
        block(this)
    }
}
