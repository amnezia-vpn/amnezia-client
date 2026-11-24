package org.amnezia.vpn.protocol.xtun

import android.net.VpnService.Builder
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.ProtocolState.RECONNECTING
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.asSequence
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.parseInetAddress
import org.amnezia.xtun.XtunException
import org.amnezia.xtun.XtunProxy
import org.amnezia.xtun.XtunTunnel
import org.json.JSONObject

private const val TAG = "Xtun"

private const val DEFAULT_MTU = 1280
private const val STATUS_CONNECTED = 2

private val INTERFACE_ADDRESSES = listOf(InetNetwork("172.19.0.1", 30), InetNetwork("fdfe:dcba:9876::1", 126))

class Xtun : Protocol() {

    private var proxy: XtunProxy? = null
    private var tunnel: XtunTunnel? = null
    private lateinit var tunConfig: JSONObject
    private lateinit var tunBuilder: Builder
    @Volatile
    private var proxyGeneration = 0
    @Volatile
    private var isTunnelConnected = false

    override fun internalInit() {}

    override suspend fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        tunConfig = config
        tunBuilder = vpnBuilder
        val generation = proxyGeneration + 1
        try {
            val newProxy = buildProxy(config, generation, protect)
            proxy = newProxy
            proxyGeneration = generation
            val newTunnel = XtunTunnel(
                { dns, includeRoutes, excludeRoutes, mtu -> createTun(dns, includeRoutes, excludeRoutes, mtu) },
                INTERFACE_ADDRESSES.map { it.toString() }.toTypedArray(),
                config.stringArray("dns")
            )
            tunnel = newTunnel
            newTunnel.attachProxy(newProxy)
            newTunnel.start()
        } catch (e: XtunException) {
            fail(e)
        }
    }

    override fun switchProxy(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        val currentTunnel = tunnel ?: return
        val oldProxy = proxy
        val generation = proxyGeneration + 1
        val newProxy = try {
            buildProxy(config, generation, protect)
        } catch (e: XtunException) {
            fail(e)
        } catch (e: Exception) {
            close()
            throw e
        }

        proxy = newProxy
        tunConfig = config
        tunBuilder = vpnBuilder
        proxyGeneration = generation
        try {
            currentTunnel.attachProxy(newProxy)
        } catch (e: XtunException) {
            oldProxy?.close()
            fail(e)
        }
        oldProxy?.close()
    }

    override fun stopVpn() {
        close()
        state.value = DISCONNECTED
    }

    override fun reconnectVpn(vpnBuilder: Builder, protect: (Int) -> Boolean) {
        tunnel?.networkChanged()
        if (isTunnelConnected) state.value = CONNECTED
    }

    private fun onStatus(status: Int) {
        Log.d(TAG, "xtun status: $status")
        isTunnelConnected = status == STATUS_CONNECTED
        if (isTunnelConnected) {
            state.value = CONNECTED
        } else if (state.value == CONNECTED) {
            state.value = RECONNECTING
        }
    }

    private fun buildProxy(config: JSONObject, generation: Int, protect: (Int) -> Boolean): XtunProxy {
        val proto = when (config.optString("proto")) {
            "awg" -> XtunProxy.Proto.AWG
            "xray" -> XtunProxy.Proto.XRAY
            else -> throw BadConfigException("The protocol is not supported by xtun")
        }
        return XtunProxy.Builder()
            .withIncludeRoutes(config.stringArray("includeRoutes"))
            .withExcludeRoutes(config.stringArray("excludeRoutes"))
            .withSocketHandler { socket -> protect(socket) }
            .withLogHandler { log -> Log.d(TAG, log) }
            .withStatusHandler { status -> if (generation == proxyGeneration) onStatus(status) }
            .build(proto, config.getString("config"))
    }

    private fun createTun(dns: Array<String>, includeRoutes: Array<String>, excludeRoutes: Array<String>, mtu: Int): Int {
        val config = tunConfig
        val vpnBuilder = tunBuilder
        val protocolConfig = ProtocolConfig.build(blockingMode = true) {
            addAddresses(INTERFACE_ADDRESSES)
            addDnsServers(dns.map(::parseInetAddress))
            addRoutes(includeRoutes.map { InetNetwork.parse(it) })
            excludeRoutes(excludeRoutes.map { InetNetwork.parse(it) })
            setMtu(if (mtu > 0) mtu else config.optInt("mtu").takeIf { it > 0 } ?: DEFAULT_MTU)
            configAppSplitTunneling(config)
        }
        buildVpnInterface(protocolConfig, vpnBuilder)

        val tunFd = vpnBuilder.establish()
        if (tunFd == null) {
            Log.e(TAG, "Create VPN interface: permission not granted or revoked")
            return -1
        }
        return tunFd.detachFd()
    }

    private fun JSONObject.stringArray(key: String): Array<String> =
        optJSONArray(key)?.asSequence<String>()?.toList()?.toTypedArray() ?: emptyArray()

    private fun fail(e: XtunException): Nothing {
        close()
        throw VpnStartException("xtun: ${e.message}", e)
    }

    private fun close() {
        isTunnelConnected = false
        val currentTunnel = tunnel
        val currentProxy = proxy
        tunnel = null
        proxy = null
        if (currentTunnel != null && currentProxy != null) {
            currentTunnel.detachProxy(currentProxy)
        }
        currentTunnel?.close()
        currentProxy?.close()
    }
}
