package org.amnezia.vpn.protocol.xtun

import android.net.VpnService.Builder
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.ProtocolState.RECONNECTING
import org.amnezia.vpn.protocol.Statistics
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
    @Volatile
    private var isTunnelConnected = false

    override val statistics: Statistics
        get() = Statistics.EMPTY_STATISTICS

    override fun internalInit() {}

    override suspend fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        val proto = when (config.optString("proto")) {
            "awg" -> XtunProxy.Proto.AWG
            "xray" -> XtunProxy.Proto.XRAY
            else -> throw BadConfigException("The protocol is not supported by xtun")
        }

        try {
            val newProxy = XtunProxy(
                proto,
                config.getString("config"),
                { socket -> protect(socket) },
                { log -> Log.d(TAG, log) },
                ::onStatus
            )
            proxy = newProxy
            val newTunnel = XtunTunnel(
                { dns, routes, mtu -> createTun(config, vpnBuilder, dns, routes, mtu) },
                INTERFACE_ADDRESSES.map { it.toString() }.toTypedArray()
            )
            tunnel = newTunnel
            newTunnel.attachProxy(newProxy)
            newTunnel.start()
        } catch (e: XtunException) {
            close()
            throw VpnStartException("xtun: ${e.message}", e)
        }
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

    private fun createTun(config: JSONObject, vpnBuilder: Builder, dns: Array<String>, routes: Array<String>, mtu: Int): Int {
        val dnsServers = dns.ifEmpty { config.getJSONArray("dns").asSequence<String>().toList().toTypedArray() }
        val protocolConfig = ProtocolConfig.build(blockingMode = true) {
            addAddresses(INTERFACE_ADDRESSES)
            addDnsServers(dnsServers.map(::parseInetAddress))
            addRoutes(routes.map { InetNetwork.parse(it) })
            setMtu(if (mtu > 0) mtu else config.optInt("mtu").takeIf { it > 0 } ?: DEFAULT_MTU)
            configSplitTunneling(config)
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
