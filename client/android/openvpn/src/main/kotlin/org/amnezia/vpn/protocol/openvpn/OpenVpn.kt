package org.amnezia.vpn.protocol.openvpn

import android.net.VpnService.Builder
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import net.openvpn.ovpn3.ClientAPI_Config
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.getLocalNetworks
import org.amnezia.vpn.util.net.parseInetAddress
import org.json.JSONObject

open class OpenVpn : Protocol() {

    private var openVpnClient: OpenVpnClient? = null
    private lateinit var scope: CoroutineScope

    override val statistics: Statistics
        get() {
            openVpnClient?.let { client ->
                val stats = client.transport_stats()
                return Statistics.build {
                    setRxBytes(stats.bytesIn)
                    setTxBytes(stats.bytesOut)
                }
            }
            return Statistics.EMPTY_STATISTICS
        }

    override fun internalInit() {
        if (!isInitialized) loadSharedLibrary(context, "ovpn3")
        if (this::scope.isInitialized) {
            scope.cancel()
        }
        scope = CoroutineScope(Dispatchers.IO)
    }

    override fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        val configBuilder = OpenVpnConfig.Builder()

        openVpnClient = OpenVpnClient(
            configBuilder = configBuilder,
            state = state,
            getLocalNetworks = { ipv6 -> getLocalNetworks(context, ipv6) },
            establish = makeEstablish(vpnBuilder),
            protect = protect,
            onError = onError
        )

        try {
            openVpnClient?.let { client ->
                val openVpnConfig = parseConfig(config)
                val evalConfig = client.eval_config(openVpnConfig)
                if (evalConfig.error) {
                    throw BadConfigException("OpenVPN config parse error: ${evalConfig.message}")
                }

                // exclude remote server ip from vpn routes
                val remoteServer = config.getString("hostName")
                val remoteServerAddress = InetNetwork(parseInetAddress(remoteServer))
                configBuilder.excludeRoute(remoteServerAddress)

                configPluggableTransport(configBuilder, config)
                configBuilder.configSplitTunneling(config)
                configBuilder.configAppSplitTunneling(config)

                scope.launch {
                    val status = client.connect()
                    if (status.error) {
                        state.value = DISCONNECTED
                        onError("OpenVpn connect() error: ${status.status}: ${status.message}")
                    }
                }
            }
        } catch (e: Exception) {
            openVpnClient = null
            throw e
        }
    }

    override fun stopVpn() {
        openVpnClient?.stop()
        openVpnClient = null
    }

    override fun reconnectVpn(vpnBuilder: Builder) {
        openVpnClient?.let {
            it.establish = makeEstablish(vpnBuilder)
            it.reconnect(0)
        }
    }

    protected open fun parseConfig(config: JSONObject): ClientAPI_Config {
        val openVpnConfig = ClientAPI_Config()
        openVpnConfig.content = config.getJSONObject("openvpn_config_data").getString("config")
        return openVpnConfig
    }

    protected open fun configPluggableTransport(configBuilder: OpenVpnConfig.Builder, config: JSONObject) {}

    private fun makeEstablish(vpnBuilder: Builder): (OpenVpnConfig.Builder) -> Int = { configBuilder ->
        val openVpnConfig = configBuilder.build()
        buildVpnInterface(openVpnConfig, vpnBuilder)

        vpnBuilder.establish().use { tunFd ->
            if (tunFd == null) {
                throw VpnStartException("Create VPN interface: permission not granted or revoked")
            }
            return@use tunFd.detachFd()
        }
    }
}
