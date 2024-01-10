package org.amnezia.vpn.util.net

import java.net.InetAddress

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
