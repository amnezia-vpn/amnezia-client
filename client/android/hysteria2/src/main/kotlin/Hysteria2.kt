package org.amnezia.vpn.protocol.hysteria2

import android.net.VpnService.Builder
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.protocol.hysteria2.libHysteria2.LibHysteria2
import org.amnezia.vpn.protocol.hysteria2.libHysteria2.SockProtector
import org.amnezia.vpn.protocol.hysteria2.libHysteria2.Tun2SocksConfig
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.parseInetAddress
import org.json.JSONObject

private const val TAG = "Hysteria2"
private const val SOCKS_PORT = 10808

class Hysteria2 : Protocol() {

    private var isRunning: Boolean = false
    override val statistics: Statistics = Statistics.EMPTY_STATISTICS

    override fun internalInit() {
        // logger is set up inside the Go library
    }

    override suspend fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        if (isRunning) {
            Log.w(TAG, "Hysteria2 already running")
            return
        }

        val hysteria2Config = config.optJSONObject("hysteria2_config_data")
            ?: throw BadConfigException("hysteria2_config_data not found")
        val protocolConfig = parseConfig(config, hysteria2Config)
        val configYaml = buildYamlConfig(config, hysteria2Config)

        start(protocolConfig, configYaml, vpnBuilder, protect)
        state.value = CONNECTED
        isRunning = true
    }

    private fun parseConfig(config: JSONObject, hysteria2Config: JSONObject): Hysteria2Config {
        return Hysteria2Config.build {
            addAddress(Hysteria2Config.DEFAULT_IPV4_ADDRESS)

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

            setSocksPort(SOCKS_PORT)
            configSplitTunneling(config)
            configAppSplitTunneling(config)
        }
    }

    private fun buildYamlConfig(config: JSONObject, hysteria2Config: JSONObject): String {
        val server = config.getString("hostName")
        val port = hysteria2Config.optInt("port", 443)
        val password = hysteria2Config.optString("password", "")
        val obfs = hysteria2Config.optString("obfs", "")
        val obfsPassword = hysteria2Config.optString("obfs_password", "")
        val sni = hysteria2Config.optString("sni", "")
        val insecure = hysteria2Config.optBoolean("insecure", false)
        val upMbps = hysteria2Config.optInt("up_mbps", 0)
        val downMbps = hysteria2Config.optInt("down_mbps", 0)

        return buildString {
            appendLine("server: $server:$port")
            appendLine("auth: $password")
            appendLine("socks5:")
            appendLine("  listen: 127.0.0.1:$SOCKS_PORT")
            appendLine("tls:")
            if (sni.isNotBlank()) appendLine("  sni: $sni")
            appendLine("  insecure: $insecure")
            if (upMbps > 0 || downMbps > 0) {
                appendLine("bandwidth:")
                if (upMbps > 0) appendLine("  up: ${upMbps} mbps")
                if (downMbps > 0) appendLine("  down: ${downMbps} mbps")
            }
            if (obfs.isNotBlank()) {
                appendLine("obfs:")
                appendLine("  type: $obfs")
                if (obfsPassword.isNotBlank()) {
                    appendLine("  salamander:")
                    appendLine("    password: $obfsPassword")
                }
            }
        }
    }

    private fun start(config: Hysteria2Config, configYaml: String, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        buildVpnInterface(config, vpnBuilder)

        SockProtector { protect(it.toInt()) }.also {
            LibHysteria2.registerSockProtector(it).isNotNullOrBlank { err ->
                throw VpnStartException("Failed to register sock protector: $err")
            }
        }

        vpnBuilder.establish().use { tunFd ->
            if (tunFd == null) {
                throw VpnStartException("Create VPN interface: permission not granted or revoked")
            }

            Log.d(TAG, "Run tun2Socks")
            runTun2Socks(config, tunFd.detachFd())

            Log.d(TAG, "Run Hysteria2")
            LibHysteria2.runClient(configYaml).isNotNullOrBlank { err ->
                LibHysteria2.stopTun2Socks()
                throw VpnStartException("Failed to start Hysteria2: $err")
            }
        }
    }

    override fun stopVpn() {
        LibHysteria2.stopClient().isNotNullOrBlank { err ->
            Log.e(TAG, "Failed to stop Hysteria2: $err")
        }
        LibHysteria2.stopTun2Socks().isNotNullOrBlank { err ->
            Log.e(TAG, "Failed to stop tun2Socks: $err")
        }

        isRunning = false
        state.value = DISCONNECTED
    }

    override fun reconnectVpn(vpnBuilder: Builder, protect: (Int) -> Boolean) {
        state.value = CONNECTED
    }

    private fun runTun2Socks(config: Hysteria2Config, fd: Int) {
        val tun2SocksConfig = Tun2SocksConfig().apply {
            mtu = config.mtu.toLong()
            proxy = "socks5://127.0.0.1:${config.socksPort}"
            device = "fd://$fd"
            logLevel = "warn"
        }
        LibHysteria2.startTun2Socks(tun2SocksConfig, fd.toLong()).isNotNullOrBlank { err ->
            throw VpnStartException("Failed to start tun2socks: $err")
        }
    }

    companion object {
        val instance: Hysteria2 by lazy { Hysteria2() }
    }
}

private fun String?.isNotNullOrBlank(block: (String) -> Unit) {
    if (!this.isNullOrBlank()) {
        block(this)
    }
}
