package org.amnezia.vpn.util.net

import java.net.Inet4Address
import java.net.InetAddress

data class InetEndpoint(val address: InetAddress, val port: Int) {

    override fun toString(): String = if (address is Inet4Address) {
        "${address.ip}:$port"
    } else {
        "[${address.ip}]:$port"
    }

    companion object {
        fun parse(data: String): InetEndpoint {
            val i = data.lastIndexOf(':')
            val address = parseInetAddress(data.substring(0, i))
            val port = data.substring(i + 1).toInt()
            return InetEndpoint(address, port)
        }
    }
}
