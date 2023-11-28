package org.amnezia.vpn.util

import android.content.Context
import android.net.ConnectivityManager
import android.net.InetAddresses
import android.net.NetworkCapabilities
import android.os.Build
import java.net.Inet4Address
import java.net.Inet6Address
import java.net.InetAddress

object NetworkUtils {

    fun getLocalNetworks(context: Context, ipv6: Boolean): List<InetNetwork> {
        val connectivityManager = context.getSystemService(ConnectivityManager::class.java)
        connectivityManager.activeNetwork?.let { network ->
            val netCapabilities = connectivityManager.getNetworkCapabilities(network)
            val linkProperties = connectivityManager.getLinkProperties(network)
            if (linkProperties == null ||
                netCapabilities == null ||
                netCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN) ||
                netCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)
            ) return emptyList()

            val addresses = mutableListOf<InetNetwork>()

            for (linkAddress in linkProperties.linkAddresses) {
                val address = linkAddress.address
                if ((!ipv6 && address is Inet4Address) || (ipv6 && address is Inet6Address)) {
                    addresses += InetNetwork(address, linkAddress.prefixLength)
                }
            }
            return addresses
        }
        return emptyList()
    }
}

data class InetNetwork(val address: InetAddress, val mask: Int) {

    constructor(address: String, mask: Int) : this(parseInetAddress(address), mask)

    constructor(address: InetAddress) : this(address, address.maxPrefixLength)

    constructor(address: String) : this(parseInetAddress(address))

    override fun toString(): String = "${address.hostAddress}/$mask"

    companion object {
        fun parse(data: String): InetNetwork {
            val split = data.split("/")
            val address = parseInetAddress(split.first())
            val mask = split.last().toInt()
            return InetNetwork(address, mask)
        }

    }
}

data class InetEndpoint(val address: InetAddress, val port: Int) {

    override fun toString(): String = "${address.hostAddress}:$port"

    companion object {
        fun parse(data: String): InetEndpoint {
            val split = data.split(":")
            val address = parseInetAddress(split.first())
            val port = split.last().toInt()
            return InetEndpoint(address, port)
        }
    }
}

private val InetAddress.maxPrefixLength: Int
    get() = if (this is Inet4Address) 32 else 128

fun parseInetAddress(address: String): InetAddress = parseNumericAddressCompat(address)

private val parseNumericAddressCompat: (String) -> InetAddress =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        InetAddresses::parseNumericAddress
    } else {
        val m = InetAddress::class.java.getMethod("parseNumericAddress", String::class.java)
        fun(address: String): InetAddress {
            return m.invoke(null, address) as InetAddress
        }
    }
