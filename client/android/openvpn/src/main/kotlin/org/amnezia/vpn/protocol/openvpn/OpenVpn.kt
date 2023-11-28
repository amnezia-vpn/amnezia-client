package org.amnezia.vpn.protocol.openvpn

import android.content.Context
import android.net.VpnService.Builder
import kotlinx.coroutines.flow.MutableStateFlow
import net.openvpn.ovpn3.ClientAPI_Config
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.util.NetworkUtils
import org.json.JSONObject

/**
 *    Config Example:
 *    {
 *     "protocol": "openvpn",
 *     "description": "Server 1",
 *     "dns1": "1.1.1.1",
 *     "dns2": "1.0.0.1",
 *     "hostName": "100.100.100.0",
 *     "splitTunnelSites": [
 *     ],
 *     "splitTunnelType": 0,
 *     "openvpn_config_data": {
 *           "config": "openVpnConfig"
 *     }
 * }
 */

class OpenVpn : Protocol() {

    private lateinit var context: Context
    private var openVpnClient: OpenVpnClient? = null

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

    override fun initialize(context: Context, state: MutableStateFlow<ProtocolState>) {
        super.initialize(context, state)
        loadSharedLibrary(context, "ovpn3")
        this.context = context
    }

    override fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean) {
        val configBuilder = OpenVpnConfig.Builder()
        openVpnClient = OpenVpnClient(
            configBuilder,
            state,
            { ipv6 -> NetworkUtils.getLocalNetworks(context, ipv6) },
            makeEstablish(configBuilder, vpnBuilder),
            protect
        )
        try {
            parseConfig(config)
            openVpnClient?.let { client ->
                val status = client.connect()
                if (status.error) {
                    throw VpnStartException("OpenVpn connect() error: ${status.status}: ${status.message}")
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

    private fun parseConfig(config: JSONObject) {
        val openVpnConfig = ClientAPI_Config()
        openVpnConfig.content = config.getJSONObject("openvpn_config_data").getString("config")
        openVpnClient?.let { client ->
            val evalConfig = client.eval_config(openVpnConfig)
            if (evalConfig.error) {
                throw BadConfigException("OpenVPN config parse error: ${evalConfig.message}")
            }
        }
    }

    private fun makeEstablish(configBuilder: OpenVpnConfig.Builder, vpnBuilder: Builder): () -> Int =
        {
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
