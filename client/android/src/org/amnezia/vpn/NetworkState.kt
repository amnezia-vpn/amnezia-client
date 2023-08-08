/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn

import android.content.Context
import android.content.Intent
import android.os.*
import android.net.*
import android.system.ErrnoException
import android.net.NetworkCapabilities
import android.net.NetworkCapabilities.NET_CAPABILITY_CAPTIVE_PORTAL
import android.net.NetworkCapabilities.NET_CAPABILITY_DUN
import android.net.NetworkCapabilities.NET_CAPABILITY_FOREGROUND
import android.net.NetworkCapabilities.NET_CAPABILITY_FOTA
import android.net.NetworkCapabilities.NET_CAPABILITY_IA
import android.net.NetworkCapabilities.NET_CAPABILITY_IMS
import android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET
import android.net.NetworkCapabilities.NET_CAPABILITY_MCX
import android.net.NetworkCapabilities.NET_CAPABILITY_MMS
import android.net.NetworkCapabilities.NET_CAPABILITY_NOT_CONGESTED
import android.net.NetworkCapabilities.NET_CAPABILITY_NOT_METERED
import android.net.NetworkCapabilities.NET_CAPABILITY_NOT_ROAMING
import android.net.NetworkCapabilities.NET_CAPABILITY_NOT_SUSPENDED
import android.net.NetworkCapabilities.NET_CAPABILITY_NOT_VPN
import android.net.NetworkCapabilities.NET_CAPABILITY_SUPL
import android.net.NetworkCapabilities.NET_CAPABILITY_TEMPORARILY_NOT_METERED
import android.net.NetworkCapabilities.NET_CAPABILITY_TRUSTED
import android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED
import android.net.NetworkCapabilities.NET_CAPABILITY_WIFI_P2P
import android.net.NetworkCapabilities.NET_CAPABILITY_XCAP
import android.net.NetworkCapabilities.TRANSPORT_BLUETOOTH
import android.net.NetworkCapabilities.TRANSPORT_CELLULAR
import android.net.NetworkCapabilities.TRANSPORT_ETHERNET
import android.net.NetworkCapabilities.TRANSPORT_LOWPAN
import android.net.NetworkCapabilities.TRANSPORT_USB
import android.net.NetworkCapabilities.TRANSPORT_VPN
import android.net.NetworkCapabilities.TRANSPORT_WIFI
import android.net.NetworkCapabilities.TRANSPORT_WIFI_AWARE
import java.io.Closeable
import java.util.EnumSet
import java.io.File
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.FileDescriptor
import java.io.IOException
import java.lang.Exception


class NetworkState(var service: VPNService)  {
    private var mService: VPNService = service
    var mCurrentContext: Context = service
    private val tag = "NetworkState"
    private var active = false
    private var listeningForDefaultNetwork = false
    private var metered = false


    enum class Transport(val systemConstant: Int) {
        BLUETOOTH(TRANSPORT_BLUETOOTH),
        CELLULAR(TRANSPORT_CELLULAR),
        ETHERNET(TRANSPORT_ETHERNET),
        VPN(TRANSPORT_VPN),
        WIFI(TRANSPORT_WIFI),
        WIFI_AWARE(if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) TRANSPORT_WIFI_AWARE else UNSUPPORTED_TRANSPORT),
        LOWPAN(if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1) TRANSPORT_LOWPAN else UNSUPPORTED_TRANSPORT),
        USB(if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) TRANSPORT_USB else UNSUPPORTED_TRANSPORT)
    }

    companion object {
        
        private const val UNSUPPORTED_TRANSPORT: Int = -1 // The TRANSPORT_* constants are non-negative.
        private const val NOT_VPN = "NOT_VPN"

	private val defaultNetworkRequest = NetworkRequest.Builder()
                .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED)
                .build()

    }

    private data class NetworkTransports(
        val network: Network,
        val transports: Set<Transport>
    )
    
    private fun getTransports(networkCapabilities: NetworkCapabilities): EnumSet<Transport> =
        Transport.values().mapNotNullTo(EnumSet.noneOf(Transport::class.java)) {
            if (networkCapabilities.hasTransport(it.systemConstant)) it else null
        }
    
    private var defaultNetworkCapabilities: Map<String, Boolean> = LinkedHashMap()
    private var defaultNetwork: NetworkTransports? = null
    val defaultNetworkTransports: Set<Transport>
        get() = defaultNetwork?.transports ?: emptySet()
        
        private val capabilitiesConstantMap = mutableMapOf(
        "MMS" to NET_CAPABILITY_MMS,
        "SUPL" to NET_CAPABILITY_SUPL,
        "DUN" to NET_CAPABILITY_DUN,
        "FOTA" to NET_CAPABILITY_FOTA,
        "IMS" to NET_CAPABILITY_IMS,
        "WIFI_P2P" to NET_CAPABILITY_WIFI_P2P,
        "IA" to NET_CAPABILITY_IA,
        "XCAP" to NET_CAPABILITY_XCAP,
        "NOT_METERED" to NET_CAPABILITY_NOT_METERED,
        "INTERNET" to NET_CAPABILITY_INTERNET,
        NOT_VPN to NET_CAPABILITY_NOT_VPN,
        "TRUSTED" to NET_CAPABILITY_TRUSTED,
        "TEMP NOT METERED" to NET_CAPABILITY_TEMPORARILY_NOT_METERED,
        "NOT SUSPENDED" to NET_CAPABILITY_MCX,
    ).apply {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            put("VALIDATED", NET_CAPABILITY_VALIDATED)
            put("CAPTIVE PORTAL", NET_CAPABILITY_CAPTIVE_PORTAL)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            put("NOT ROAMING", NET_CAPABILITY_NOT_ROAMING)
            put("TRUSTED", NET_CAPABILITY_FOREGROUND)
            put("NOT CONGESTED", NET_CAPABILITY_NOT_CONGESTED)
            put("NOT SUSPENDED", NET_CAPABILITY_NOT_SUSPENDED)
        }
    } as Map<String, Int>

    
    
    private val connectivity by lazy { mCurrentContext.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager }

    private var mLastNetworkCapabilities: String? = null

    private val defaultNetworkCallback = object : ConnectivityManager.NetworkCallback() {
        override fun onAvailable(network: Network) {
            super.onAvailable(network)
            
            
            Log.i(tag, "onAvailable $network")
        }
        override fun onCapabilitiesChanged(network: Network, networkCapabilities: NetworkCapabilities) {
            val newCapabilities = capabilitiesConstantMap.mapValues {
                networkCapabilities.hasCapability(it.value)
            }
            val newTransports = getTransports(networkCapabilities)
            val capabilitiesChanged = defaultNetworkCapabilities != newCapabilities
            if (defaultNetwork?.network != network ||
                defaultNetwork?.transports != newTransports ||
                capabilitiesChanged
            ) {
                Log.i(
                    tag,
                    "default network: $network; transports: ${newTransports.joinToString(", ")}; " +
                        "capabilities: $newCapabilities"
                )
                defaultNetwork = NetworkTransports(network, newTransports)
            }
            if (capabilitiesChanged) {
               mService.networkChange()
               
               Log.i(tag, "onCapabilitiesChanged capabilitiesChanged $network $networkCapabilities")
                defaultNetworkCapabilities = newCapabilities
            }
            super.onCapabilitiesChanged(network, networkCapabilities)
        }
        
        override fun onBlockedStatusChanged(network: Network, blocked: Boolean) {
	      super.onBlockedStatusChanged(network, blocked)
	      Log.i(tag, "onBlockedStatusChanged $network $blocked")
	    }

        
        override fun onLost(network: Network) {
            super.onLost(network)
            Log.i(tag, "onLost")
        }
    }
    
    fun bindNetworkListener() {
        if (Build.VERSION.SDK_INT >= 28) {
            // we want REQUEST here instead of LISTEN
            connectivity.requestNetwork(defaultNetworkRequest, defaultNetworkCallback)
            listeningForDefaultNetwork = true
        }
    }
    
    fun unBindNetworkListener() {
        if (Build.VERSION.SDK_INT >= 28) {
            connectivity.unregisterNetworkCallback(defaultNetworkCallback)
            listeningForDefaultNetwork = false
        }
    }
    
    
    
    
}
