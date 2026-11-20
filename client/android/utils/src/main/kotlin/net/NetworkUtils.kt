package org.amnezia.vpn.util.net

import android.content.Context
import android.net.ConnectivityManager
import android.net.InetAddresses
import android.net.NetworkCapabilities
import android.os.Build
import androidx.core.content.getSystemService
import java.lang.reflect.InvocationTargetException
import java.net.Inet4Address
import java.net.Inet6Address
import java.net.InetAddress

fun getLocalNetworks(context: Context, ipv6: Boolean): List<InetNetwork> {
    val connectivityManager = context.getSystemService<ConnectivityManager>()!!
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

fun parseInetAddress(address: String): InetAddress = InetAddress.getByName(address)

/** True for an IPv4 or IPv6 literal, false for a host name. Never queries DNS. */
fun isNumericAddress(address: String): Boolean =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        InetAddresses.isNumericAddress(address)
    } else {
        address.contains(':') || IPV4_LITERAL.matches(address)
    }

private val IPV4_LITERAL = Regex("""\d{1,3}(\.\d{1,3}){3}""")

private val parseNumericAddressCompat: (String) -> InetAddress =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        InetAddresses::parseNumericAddress
    } else {
        try {
            val m = InetAddress::class.java.getMethod("parseNumericAddress", String::class.java)
            fun(address: String): InetAddress {
                try {
                    return m.invoke(null, address) as InetAddress
                } catch (e: InvocationTargetException) {
                    throw e.cause ?: e
                }
            }
        } catch (_: NoSuchMethodException) {
            fun(address: String): InetAddress {
                return InetAddress.getByName(address)
            }
        }
    }

internal fun convertIpv6ToCanonicalForm(ipv6: String): String = ipv6
    .replace("((?:(?:^|:)0+\\b){2,}):?(?!\\S*\\b\\1:0+\\b)(\\S*)".toRegex(), "::$2")

val InetAddress.ip: String
    get() = if (this is Inet4Address) {
        hostAddress!!
    } else {
        convertIpv6ToCanonicalForm(hostAddress!!)
    }
