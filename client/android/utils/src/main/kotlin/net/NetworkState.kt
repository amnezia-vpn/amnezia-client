package org.amnezia.vpn.util.net

import android.content.Context
import android.net.ConnectivityManager
import android.net.ConnectivityManager.NetworkCallback
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET
import android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED
import android.net.NetworkRequest
import android.os.Handler
import androidx.core.content.getSystemService
import kotlin.LazyThreadSafetyMode.NONE
import org.amnezia.vpn.util.Log

private const val TAG = "NetworkState"

class NetworkState(
    private val context: Context,
    private val onNetworkChange: () -> Unit
) {
    private var currentNetwork: Network? = null
    private var validated: Boolean = false
    private var isListenerBound = false

    private val handler: Handler by lazy(NONE) {
        Handler(context.mainLooper)
    }

    private val connectivityManager: ConnectivityManager by lazy(NONE) {
        context.getSystemService<ConnectivityManager>()!!
    }

    private val networkRequest: NetworkRequest by lazy(NONE) {
        NetworkRequest.Builder()
            .addCapability(NET_CAPABILITY_INTERNET)
            .build()
    }

    private val networkCallback: NetworkCallback by lazy(NONE) {
        object : NetworkCallback() {
            override fun onAvailable(network: Network) {
                Log.v(TAG, "onAvailable: $network")
            }

            override fun onCapabilitiesChanged(network: Network, networkCapabilities: NetworkCapabilities) {
                Log.v(TAG, "onCapabilitiesChanged: $network, $networkCapabilities")
                handler.post {
                    checkNetworkState(network, networkCapabilities)
                }
            }

            private fun checkNetworkState(network: Network, networkCapabilities: NetworkCapabilities) {
                if (currentNetwork == null) {
                    currentNetwork = network
                    validated = networkCapabilities.hasCapability(NET_CAPABILITY_VALIDATED)
                } else {
                    if (currentNetwork != network) {
                        currentNetwork = network
                        validated = false
                    }
                    if (!validated) {
                        validated = networkCapabilities.hasCapability(NET_CAPABILITY_VALIDATED)
                        if (validated) onNetworkChange()
                    }
                }
            }

            override fun onBlockedStatusChanged(network: Network, blocked: Boolean) {
                Log.v(TAG, "onBlockedStatusChanged: $network, $blocked")
            }

            override fun onLost(network: Network) {
                Log.v(TAG, "onLost: $network")
            }
        }
    }

    fun bindNetworkListener() {
        if (isListenerBound) return
        Log.d(TAG, "Bind network listener")
        connectivityManager.requestNetwork(networkRequest, networkCallback)
        isListenerBound = true
    }

    fun unbindNetworkListener() {
        if (!isListenerBound) return
        Log.d(TAG, "Unbind network listener")
        connectivityManager.unregisterNetworkCallback(networkCallback)
        isListenerBound = false
        currentNetwork = null
        validated = false
    }
}
