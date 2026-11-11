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

fun parseInetAddress(address: String): InetAddress = InetAddress.getByName(address)

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
