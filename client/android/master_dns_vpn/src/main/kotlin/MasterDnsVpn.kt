package org.amnezia.vpn.protocol.masterdnsvpn

import android.net.VpnService.Builder
import java.io.IOException
import java.net.InetSocketAddress
import java.net.Socket
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.protocol.xray.libXray.LibXray
import org.amnezia.vpn.protocol.xray.libXray.Tun2SocksConfig
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.parseInetAddress
import org.json.JSONObject

private const val TAG = "MasterDnsVpn"

/**
 * Android-side glue that bridges the Amnezia VpnService lifecycle to the
 * native MasterDnsVPN engine.
 *
 * Architecture:
 *
 *   AmneziaVpnService → MasterDnsVpn (this class)
 *       → MasterDnsVpnNative.nativeStart(configJson)        // C++ engine spins up,
 *                                                           // binds 127.0.0.1:<port>
 *       → poll MasterDnsVpnNative.nativeSocksPort()         // wait for listener
 *       → buildVpnInterface(...)                            // VpnService.Builder TUN
 *       → LibXray.startTun2Socks(...)                       // tun2socks dials SOCKS5
 *
 * The native engine and tun2socks both live in the same process — no IPC,
 * no subprocess, no bundled binary. tun2socks comes from libxray.aar (a
 * dependency Amnezia already ships for the Xray protocol); we reuse its
 * tun2socks engine rather than vendoring a second copy.
 */
class MasterDnsVpn : Protocol() {

    private var isRunning: Boolean = false
    override val statistics: Statistics = Statistics.EMPTY_STATISTICS

    override fun internalInit() {
        // No persistent native init: the engine instance is created fresh
        // on every startVpn() and discarded on stopVpn().
    }

    override suspend fun startVpn(
        config: JSONObject,
        vpnBuilder: Builder,
        protect: (Int) -> Boolean
    ) {
        if (isRunning) {
            Log.w(TAG, "MasterDnsVpn already running")
            return
        }

        val configData = config.optJSONObject("masterdnsvpn_config_data")
            ?: throw BadConfigException("masterdnsvpn_config_data not found")

        if (!MasterDnsVpnNative.nativeStart(configData.toString())) {
            val err = MasterDnsVpnNative.nativeLastError()
            throw VpnStartException("Failed to start MasterDnsVpn engine: $err")
        }

        // Engine binds the SOCKS5 listener asynchronously after start();
        // tun2socks must not race the listener. Poll until non-zero or
        // we hit the deadline.
        val socksPort = waitForSocksListener()
        if (socksPort == 0) {
            MasterDnsVpnNative.nativeStop()
            throw VpnStartException("MasterDnsVpn engine did not bind a SOCKS5 listener")
        }
        Log.d(TAG, "MasterDnsVpn engine listening on 127.0.0.1:$socksPort")

        val mdnsvpnConfig = parseConfig(config, configData, socksPort)

        var startupSucceeded = false
        try {
            buildVpnInterface(mdnsvpnConfig, vpnBuilder)
            vpnBuilder.establish().use { tunFd ->
                if (tunFd == null) {
                    throw VpnStartException(
                        "Create VPN interface: permission not granted or revoked"
                    )
                }
                Log.d(TAG, "Run tun2Socks (SOCKS5 backend = native MasterDnsVpn engine)")
                runTun2Socks(mdnsvpnConfig, tunFd.detachFd())
            }
            startupSucceeded = true
        } finally {
            if (!startupSucceeded) {
                MasterDnsVpnNative.nativeStop()
            }
        }

        state.value = CONNECTED
        isRunning = true
    }

    override fun stopVpn() {
        LibXray.stopTun2Socks().isNotNullOrBlank { err ->
            Log.e(TAG, "Failed to stop tun2Socks: $err")
        }
        MasterDnsVpnNative.nativeStop()
        isRunning = false
        state.value = DISCONNECTED
    }

    override fun reconnectVpn(vpnBuilder: Builder, protect: (Int) -> Boolean) {
        // Engine handles its own resilience (ARQ retries, per-resolver
        // failover). From Amnezia's perspective the tunnel is up as long
        // as tun2socks + the engine are alive.
        state.value = CONNECTED
    }

    private fun parseConfig(
        config: JSONObject,
        configData: JSONObject,
        socksPort: Int
    ): MasterDnsVpnConfig {
        return MasterDnsVpnConfig.build {
            addAddress(MasterDnsVpnConfig.DEFAULT_IPV4_ADDRESS)

            config.optString("dns1").let {
                if (it.isNotBlank()) addDnsServer(parseInetAddress(it))
            }
            config.optString("dns2").let {
                if (it.isNotBlank()) addDnsServer(parseInetAddress(it))
            }

            // Default-route both v4 and v6 — MasterDnsVPN is intended as a
            // full-tunnel transport for environments where only DNS leaves
            // the network.
            addRoute(InetNetwork("0.0.0.0", 0))
            addRoute(InetNetwork("2000::0", 3))

            // Carve the operator's server hostname out of the tunnel so the
            // underlying DNS queries can still reach it.
            config.optString("hostName").let {
                if (it.isNotBlank()) excludeRoute(InetNetwork(it, 32))
            }

            config.optString("mtu").let {
                if (it.isNotBlank()) setMtu(it.toInt())
            }

            setSocksPort(socksPort)

            configSplitTunneling(config)
            configAppSplitTunneling(config)
        }
    }

    private fun runTun2Socks(config: MasterDnsVpnConfig, fd: Int) {
        // Engine binds 127.0.0.1 only; no auth needed (the device-to-engine
        // hop never leaves the loopback interface).
        val proxyUrl = "socks5://127.0.0.1:${config.socksPort}"
        val tun2SocksConfig = Tun2SocksConfig().apply {
            mtu = config.mtu.toLong()
            proxy = proxyUrl
            device = "fd://$fd"
            logLevel = "warn"
        }
        LibXray.startTun2Socks(tun2SocksConfig, fd.toLong()).isNotNullOrBlank { err ->
            throw VpnStartException("Failed to start tun2socks for MasterDnsVpn: $err")
        }
    }

    private fun waitForSocksListener(): Int {
        // Up to 60 s — the engine's MTU discovery + handshake can take a
        // bit on a fresh connect. Once the SOCKS5 port is non-zero we
        // additionally probe via TCP connect to confirm the listener is
        // accept()ing (handles the brief window between bind and listen).
        val deadline = System.currentTimeMillis() + SOCKS_WAIT_TIMEOUT_MS
        while (System.currentTimeMillis() < deadline) {
            val port = MasterDnsVpnNative.nativeSocksPort()
            if (port > 0) {
                if (probeSocks(port)) {
                    return port
                }
            }
            try {
                Thread.sleep(SOCKS_PROBE_INTERVAL_MS.toLong())
            } catch (_: InterruptedException) {
                Thread.currentThread().interrupt()
                return 0
            }
        }
        return 0
    }

    /**
     * Connect-probe the native engine's SOCKS5 listener on 127.0.0.1.
     *
     * This deliberately uses a plain [Socket] (no TLS): the destination is
     * always the local loopback address bound by the engine's own SOCKS5
     * server in the same process. The probe never crosses a network
     * interface and there is no transport-security concern. The connection
     * is closed immediately on success without exchanging any bytes.
     */
    @Suppress("UnsafeSocketUsage")
    private fun probeSocks(port: Int): Boolean {
        return try {
            Socket().use { sock ->
                sock.connect(
                    InetSocketAddress(LOOPBACK_ADDRESS, port),
                    SOCKS_PROBE_INTERVAL_MS,
                )
                sock.isConnected
            }
        } catch (_: IOException) {
            false
        }
    }

    /**
     * Process-wide singleton plus the timing constants for the SOCKS5
     * listener handshake. Mirrors the pattern used by the Xray protocol
     * module in the same package.
     */
    companion object {
        private const val SOCKS_WAIT_TIMEOUT_MS = 60_000
        private const val SOCKS_PROBE_INTERVAL_MS = 250
        private const val LOOPBACK_ADDRESS = "127.0.0.1"

        val instance: MasterDnsVpn by lazy { MasterDnsVpn() }
    }
}

private fun String?.isNotNullOrBlank(block: (String) -> Unit) {
    if (!this.isNullOrBlank()) {
        block(this)
    }
}
