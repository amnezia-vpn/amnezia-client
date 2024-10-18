package org.amnezia.vpn.protocol

import android.content.Context
import android.net.IpPrefix
import android.net.VpnService
import android.net.VpnService.Builder
import android.os.Build
import android.system.OsConstants
import androidx.annotation.RequiresApi
import kotlinx.coroutines.flow.MutableStateFlow
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.net.InetNetwork
import org.json.JSONObject

private const val TAG = "Protocol"

const val VPN_SESSION_NAME = "AmneziaVPN"

private const val SPLIT_TUNNEL_DISABLE = 0
private const val SPLIT_TUNNEL_INCLUDE = 1
private const val SPLIT_TUNNEL_EXCLUDE = 2

abstract class Protocol {

    abstract val statistics: Statistics
    protected lateinit var context: Context
    protected lateinit var state: MutableStateFlow<ProtocolState>
    protected lateinit var onError: (String) -> Unit
    protected var isInitialized: Boolean = false

    fun initialize(context: Context, state: MutableStateFlow<ProtocolState>, onError: (String) -> Unit) {
        this.context = context
        this.state = state
        this.onError = onError
        internalInit()
        isInitialized = true
    }

    protected abstract fun internalInit()

    abstract suspend fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean)

    abstract fun stopVpn()

    abstract fun reconnectVpn(vpnBuilder: Builder)

    protected fun ProtocolConfig.Builder.configSplitTunneling(config: JSONObject) {
        if (!allowSplitTunneling) {
            Log.i(TAG, "Global address split tunneling is prohibited, " +
                "only tunneling from the protocol config is used")
            return
        }

        val splitTunnelType = config.optInt("splitTunnelType")
        if (splitTunnelType == SPLIT_TUNNEL_DISABLE) return
        val splitTunnelSites = config.getJSONArray("splitTunnelSites")
        val addressHandlerFunc = when (splitTunnelType) {
            SPLIT_TUNNEL_INCLUDE -> ::includeAddress
            SPLIT_TUNNEL_EXCLUDE -> ::excludeAddress

            else -> throw BadConfigException("Unexpected value of the 'splitTunnelType' parameter: $splitTunnelType")
        }

        for (i in 0 until splitTunnelSites.length()) {
            val address = InetNetwork.parse(splitTunnelSites.getString(i))
            addressHandlerFunc(address)
        }
    }

    protected fun ProtocolConfig.Builder.configAppSplitTunneling(config: JSONObject) {
        val splitTunnelType = config.optInt("appSplitTunnelType")
        if (splitTunnelType == SPLIT_TUNNEL_DISABLE) return
        val splitTunnelApps = config.getJSONArray("splitTunnelApps")
        val appHandlerFunc = when (splitTunnelType) {
            SPLIT_TUNNEL_INCLUDE -> ::includeApplication
            SPLIT_TUNNEL_EXCLUDE -> ::excludeApplication

            else -> throw BadConfigException("Unexpected value of the 'appSplitTunnelType' parameter: $splitTunnelType")
        }

        for (i in 0 until splitTunnelApps.length()) {
            appHandlerFunc(splitTunnelApps.getString(i))
        }
    }

    protected open fun buildVpnInterface(config: ProtocolConfig, vpnBuilder: Builder) {
        vpnBuilder.setSession(VPN_SESSION_NAME)

        for (addr in config.addresses) {
            Log.d(TAG, "addAddress: $addr")
            vpnBuilder.addAddress(addr)
        }

        for (addr in config.dnsServers) {
            Log.d(TAG, "addDnsServer: $addr")
            vpnBuilder.addDnsServer(addr)
        }
        // fix for Samsung android ignoring DNS servers outside the VPN route range
        if (Build.BRAND == "samsung") {
            for (addr in config.dnsServers) {
                Log.d(TAG, "addRoute: $addr")
                vpnBuilder.addRoute(InetNetwork(addr))
            }
        }

        config.searchDomain?.let {
            Log.d(TAG, "addSearchDomain: $it")
            vpnBuilder.addSearchDomain(it)
        }

        for ((inetNetwork, include) in config.routes) {
            if (include) {
                Log.d(TAG, "addRoute: $inetNetwork")
                vpnBuilder.addRoute(inetNetwork)
            } else {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    Log.d(TAG, "excludeRoute: $inetNetwork")
                    vpnBuilder.excludeRoute(inetNetwork)
                } else {
                    Log.e(TAG, "Trying to exclude route $inetNetwork on old Android")
                }
            }
        }

        for (app in config.includedApplications) {
            Log.d(TAG, "addAllowedApplication")
            vpnBuilder.addAllowedApplication(app)
        }

        for (app in config.excludedApplications) {
            Log.d(TAG, "addDisallowedApplication")
            vpnBuilder.addDisallowedApplication(app)
        }

        Log.d(TAG, "setMtu: ${config.mtu}")
        vpnBuilder.setMtu(config.mtu)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            config.httpProxy?.let {
                Log.d(TAG, "setHttpProxy: $it")
                vpnBuilder.setHttpProxy(it)
            }
        }

        if (config.allowAllAF) {
            Log.d(TAG, "allowFamily")
            vpnBuilder.allowFamily(OsConstants.AF_INET)
            vpnBuilder.allowFamily(OsConstants.AF_INET6)
        }

        Log.d(TAG, "setBlocking: ${config.blockingMode}")
        vpnBuilder.setBlocking(config.blockingMode)
        vpnBuilder.setUnderlyingNetworks(null)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
            vpnBuilder.setMetered(false)
    }
}

private fun VpnService.Builder.addAddress(addr: InetNetwork) = addAddress(addr.address, addr.mask)
private fun VpnService.Builder.addRoute(addr: InetNetwork) = addRoute(addr.address, addr.mask)

@RequiresApi(Build.VERSION_CODES.TIRAMISU)
private fun VpnService.Builder.excludeRoute(addr: InetNetwork) = excludeRoute(IpPrefix(addr.address, addr.mask))
