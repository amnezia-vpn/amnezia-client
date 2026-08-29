package org.amnezia.vpn.protocol.dnstt

import android.net.VpnService.Builder
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.ProtocolState.RECONNECTING
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.util.LibraryLoader.loadSharedLibrary
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.parseInetAddress
import org.json.JSONObject

private const val TAG = "Dnstt"

/** dnstt cannot carry a payload smaller than this; see libdnstt's minMtu. */
private const val MIN_TUNNEL_MTU = 80

class Dnstt : Protocol() {

    private var isRunning: Boolean = false
    override val statistics: Statistics = Statistics.EMPTY_STATISTICS

    override fun internalInit() {
        if (!isInitialized) {
            loadSharedLibrary(context, "dnstt")
        }
    }

    override suspend fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        if (isRunning) {
            Log.w(TAG, "DNSTT already running")
            return
        }

        val dnsttConfigData = config.optJSONObject("dnstt_config_data")
            ?: throw BadConfigException("dnstt_config_data not found")

        val dnsttConfig = parseConfig(config, dnsttConfigData)
        start(dnsttConfig, vpnBuilder, protect)
        state.value = CONNECTED
        isRunning = true
    }

    private fun parseConfig(config: JSONObject, dnsttConfigData: JSONObject): DnsttConfig {
        return DnsttConfig.build {
            addAddress(DnsttConfig.DEFAULT_IPV4_ADDRESS)

            config.optString("dns1").let {
                if (it.isNotBlank()) addDnsServer(parseInetAddress(it))
            }
            config.optString("dns2").let {
                if (it.isNotBlank()) addDnsServer(parseInetAddress(it))
            }

            addRoute(InetNetwork("0.0.0.0", 0))
            addRoute(InetNetwork("2000::0", 3))

            // No route is excluded for the tunnel endpoint: the resolver is
            // reached by IP that is only known inside libdnstt, which keeps its
            // own sockets outside the VPN via DnsttNative.protector.

            val domain = dnsttConfigData.optString("domain")
            if (domain.isBlank()) {
                throw BadConfigException("DNSTT domain is empty")
            }
            setDomain(domain)

            val resolvers = dnsttConfigData.optString("resolvers")
            if (resolvers.isBlank()) {
                throw BadConfigException("DNSTT resolvers list is empty")
            }
            setResolvers(resolvers)

            setBootstrapIp(dnsttConfigData.optString("bootstrap_ip", ""))

            val publicKey = dnsttConfigData.optString("public_key")
            if (publicKey.length != 64) {
                throw BadConfigException("Invalid public key (expected 64 hex characters)")
            }
            setPublicKey(publicKey)

            // The payload MTU only validates the domain here; libdnstt derives
            // and enforces the authoritative value itself.
            val tunnelMtu = DnsttNative.calculateMtu(domain)
            if (tunnelMtu < MIN_TUNNEL_MTU) {
                throw BadConfigException(
                    "Domain '$domain' leaves only $tunnelMtu bytes of payload, at least $MIN_TUNNEL_MTU are required"
                )
            }
            Log.i(TAG, "dnstt payload MTU for $domain is $tunnelMtu bytes")

            configSplitTunneling(config)
            configAppSplitTunneling(config)
        }
    }

    private fun start(config: DnsttConfig, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        buildVpnInterface(config, vpnBuilder)

        DnsttNative.protector = protect
        // libdnstt rebuilds the session on its own when the resolver path
        // breaks; reflect that in the UI instead of staying CONNECTED.
        DnsttNative.stateListener = { tunnelState ->
            when (tunnelState) {
                "connected" -> state.value = CONNECTED
                "reconnecting" -> state.value = RECONNECTING
                "disconnected" -> if (isRunning) state.value = DISCONNECTED
                else -> Log.w(TAG, "Unknown tunnel state: $tunnelState")
            }
        }

        vpnBuilder.establish().use { tunFd ->
            if (tunFd == null) {
                clearNativeHooks()
                throw VpnStartException("Create VPN interface: permission not granted or revoked")
            }

            Log.i(TAG, "Starting libdnstt tunnel with domain: ${config.domain}")
            // detachFd: libdnstt owns the descriptor from here on and closes it
            // itself, so it must outlive this `use` block.
            val error = DnsttNative.startTunnel(
                tunFd.detachFd(),
                config.mtu,
                config.domain,
                config.resolvers,
                config.bootstrapIp,
                config.publicKey
            )
            if (error != null) {
                clearNativeHooks()
                throw VpnStartException("Failed to start dnstt tunnel: $error")
            }
        }
    }

    override fun stopVpn() {
        if (!isRunning) {
            return
        }

        try {
            DnsttNative.stopTunnel()?.let { Log.e(TAG, "Error stopping dnstt tunnel: $it") }
        } catch (e: Exception) {
            Log.e(TAG, "Error stopping dnstt tunnel: ${e.message}")
        } finally {
            clearNativeHooks()
        }

        state.value = DISCONNECTED
        isRunning = false
    }

    override fun reconnectVpn(vpnBuilder: Builder, protect: (Int) -> Boolean) {
        state.value = CONNECTED
    }

    private fun clearNativeHooks() {
        DnsttNative.protector = null
        DnsttNative.stateListener = null
    }

    companion object {
        val instance: Dnstt by lazy { Dnstt() }
    }
}
