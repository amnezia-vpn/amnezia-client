package org.amnezia.vpn.protocol.wireguard

import android.net.VpnService.Builder
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.amnezia.awg.GoBackend
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VpnException
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.util.LibraryLoader.loadSharedLibrary
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.asSequence
import org.amnezia.vpn.util.net.InetEndpoint
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.parseInetAddress
import org.amnezia.vpn.util.optStringOrNull
import org.json.JSONObject

private const val TAG = "Wireguard"

open class Wireguard : Protocol() {

    private var tunnelHandle: Int = -1
    private var config: WireguardConfig? = null // save config for reconnect
    protected open val ifName: String = "amn0"
    private lateinit var scope: CoroutineScope
    private var statusJob: Job? = null

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
        if (this::scope.isInitialized) {
            scope.cancel()
        }
        scope = CoroutineScope(Dispatchers.IO)
    }

    override suspend fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        val wireguardConfig = parseConfig(config)
        start(wireguardConfig, vpnBuilder, protect)
        this.config = wireguardConfig
    }

    protected open fun parseConfig(config: JSONObject): WireguardConfig {
        val configData = config.getJSONObject("wireguard_config_data")
        return WireguardConfig.build {
            configWireguard(config, configData)
            configSplitTunneling(config)
            configAppSplitTunneling(config)
        }
    }

    protected fun WireguardConfig.Builder.configWireguard(config: JSONObject, configData: JSONObject) {
        configData.getString("client_ip").split(",").map { address ->
            InetNetwork.parse(address.trim())
        }.forEach(::addAddress)

        config.optStringOrNull("dns1")?.let { dns ->
            addDnsServer(parseInetAddress(dns.trim()))
        }

        config.optStringOrNull("dns2")?.let { dns ->
            addDnsServer(parseInetAddress(dns.trim()))
        }

        configData.optStringOrNull("mtu")?.let { setMtu(it.toInt()) }
        configData.getString("client_priv_key").let { setPrivateKeyHex(it.base64ToHex()) }

        if (configData.optBoolean("isObfuscationEnabled")) {
            setUseProtocolExtension(true)
            configExtensionParameters(configData)
        }

        val defRoutes = hashSetOf(InetNetwork("0.0.0.0", 0), InetNetwork("::", 0))
        val peersArray = configData.optJSONArray("peers")

        if (peersArray != null && peersArray.length() > 0) {
            // Multi-peer: collect union of all peers' allowed IPs for the VPN interface routing table
            val allRoutes = hashSetOf<InetNetwork>()
            for (i in 0 until peersArray.length()) {
                peersArray.getJSONObject(i).getJSONArray("allowed_ips").asSequence<String>()
                    .map { InetNetwork.parse(it.trim()) }.forEach(allRoutes::add)
            }
            if (allRoutes.any { it !in defRoutes }) disableSplitTunneling()
            addRoutes(allRoutes)

            // Primary peer from first entry
            val firstPeer = peersArray.getJSONObject(0)
            val firstAllowedIps = firstPeer.getJSONArray("allowed_ips").asSequence<String>()
                .map { InetNetwork.parse(it.trim()) }.toList()
            setPeerAllowedIps(firstAllowedIps)
            setEndpoint(InetEndpoint(parseInetAddress(firstPeer.getString("hostName").trim()), firstPeer.getInt("port")))
            firstPeer.optStringOrNull("persistent_keep_alive")?.let { setPersistentKeepalive(it.toInt()) }
            firstPeer.getString("server_pub_key").let { setPublicKeyHex(it.base64ToHex()) }
            firstPeer.optStringOrNull("psk_key")?.let { setPreSharedKeyHex(it.base64ToHex()) }

            // Additional peers
            for (i in 1 until peersArray.length()) {
                val peerData = peersArray.getJSONObject(i)
                val peerAllowedIps = peerData.getJSONArray("allowed_ips").asSequence<String>()
                    .map { InetNetwork.parse(it.trim()) }.toList()
                addPeer(
                    PeerConfig(
                        publicKeyHex = peerData.getString("server_pub_key").base64ToHex(),
                        preSharedKeyHex = peerData.optStringOrNull("psk_key")?.base64ToHex(),
                        persistentKeepalive = peerData.optStringOrNull("persistent_keep_alive")?.toInt() ?: 0,
                        endpoint = InetEndpoint(parseInetAddress(peerData.getString("hostName").trim()), peerData.getInt("port")),
                        allowedIps = peerAllowedIps
                    )
                )
            }
        } else {
            // Single peer (original behavior)
            val routes = hashSetOf<InetNetwork>()
            configData.getJSONArray("allowed_ips").asSequence<String>().map { route ->
                InetNetwork.parse(route.trim())
            }.forEach(routes::add)
            if (routes.any { it !in defRoutes }) disableSplitTunneling()
            addRoutes(routes)

            val host = configData.getString("hostName").let { parseInetAddress(it.trim()) }
            val port = configData.getInt("port")
            setEndpoint(InetEndpoint(host, port))
            configData.optStringOrNull("persistent_keep_alive")?.let { setPersistentKeepalive(it.toInt()) }
            configData.getString("server_pub_key").let { setPublicKeyHex(it.base64ToHex()) }
            configData.optStringOrNull("psk_key")?.let { setPreSharedKeyHex(it.base64ToHex()) }
        }
    }

    protected fun WireguardConfig.Builder.configExtensionParameters(configData: JSONObject) {
        configData.optStringOrNull("Jc")?.let { setJc(it.toInt()) }
        configData.optStringOrNull("Jmin")?.let { setJmin(it.toInt()) }
        configData.optStringOrNull("Jmax")?.let { setJmax(it.toInt()) }
        configData.optStringOrNull("S1")?.let { setS1(it.toInt()) }
        configData.optStringOrNull("S2")?.let { setS2(it.toInt()) }
        configData.optStringOrNull("S3")?.let { setS3(it.toInt()) }
        configData.optStringOrNull("S4")?.let { setS4(it.toInt()) }
        configData.optStringOrNull("H1")?.trim()?.let { if (it.isNotEmpty()) setH1(it) }
        configData.optStringOrNull("H2")?.trim()?.let { if (it.isNotEmpty()) setH2(it) }
        configData.optStringOrNull("H3")?.trim()?.let { if (it.isNotEmpty()) setH3(it) }
        configData.optStringOrNull("H4")?.trim()?.let { if (it.isNotEmpty()) setH4(it) }
        configData.optStringOrNull("I1")?.let { setI1(it) }
        configData.optStringOrNull("I2")?.let { setI2(it) }
        configData.optStringOrNull("I3")?.let { setI3(it) }
        configData.optStringOrNull("I4")?.let { setI4(it) }
        configData.optStringOrNull("I5")?.let { setI5(it) }
    }

    private fun start(
        config: WireguardConfig,
        vpnBuilder: Builder,
        protect: (Int) -> Boolean,
        stopExistingVpn: Boolean = false
    ) {
        if (!stopExistingVpn && tunnelHandle != -1) {
            Log.w(TAG, "Tunnel already up")
            return
        }

        buildVpnInterface(config, vpnBuilder)

        vpnBuilder.establish().use { tunFd ->
            if (stopExistingVpn && tunnelHandle != -1) {
                turnOffVpn()
            }
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
        launchStatusJob()
    }

    private fun launchStatusJob() {
        Log.d(TAG, "Launch status job")
        statusJob = scope.launch {
            while (true) {
                val lastHandshake = getLastHandshake()
                Log.v(TAG, "lastHandshake=$lastHandshake")
                if (lastHandshake == 0L) {
                    delay(1000)
                    continue
                }
                if (lastHandshake == -2L || lastHandshake > 0L) state.value = CONNECTED
                else if (lastHandshake == -1L) state.value = DISCONNECTED
                statusJob = null
                break
            }
        }
    }

    private fun getLastHandshake(): Long {
        if (tunnelHandle == -1) {
            Log.e(TAG, "Trying to get config of a non-existent tunnel")
            return -1
        }
        val config = GoBackend.awgGetConfig(tunnelHandle)
        if (config == null) {
            Log.e(TAG, "Failed to get tunnel config")
            return -2
        }
        // For multi-peer: take the max handshake time across all peers (any connected peer = tunnel active)
        val lastHandshake = config.lines()
            .filter { it.startsWith("last_handshake_time_sec=") }
            .mapNotNull { it.substring(24).toLongOrNull() }
            .maxOrNull()
        if (lastHandshake == null) {
            Log.e(TAG, "Failed to get last_handshake_time_sec")
            return -2
        }
        return lastHandshake
    }

    private fun turnOffVpn() {
        statusJob?.cancel()
        statusJob = null
        val handleToClose = tunnelHandle
        tunnelHandle = -1
        GoBackend.awgTurnOff(handleToClose)
    }

    override fun stopVpn() {
        if (tunnelHandle == -1) {
            Log.w(TAG, "Tunnel already down")
            return
        }
        turnOffVpn()
        state.value = DISCONNECTED
    }

    override fun reconnectVpn(vpnBuilder: Builder, protect: (Int) -> Boolean) {
        val config = this.config ?: throw VpnException("Reconnect config is empty")
        start(config, vpnBuilder, protect, true)
    }
}
