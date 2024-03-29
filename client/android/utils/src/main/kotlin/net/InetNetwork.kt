package org.amnezia.vpn.util.net

import java.net.Inet4Address
import java.net.InetAddress

data class InetNetwork(val address: InetAddress, val mask: Int) {

    constructor(address: String, mask: Int) : this(parseInetAddress(address), mask)

    constructor(address: InetAddress) : this(address, address.maxPrefixLength)

    override fun toString(): String = "${address.hostAddress}/$mask"

    companion object {
        fun parse(data: String): InetNetwork {
            val split = data.split("/")
            val address = parseInetAddress(split.first())
            if (split.size == 1) return InetNetwork(address)
            val mask = split.last().toInt()
            return InetNetwork(address, mask)
        }
    }
}

private val InetAddress.maxPrefixLength: Int
    get() = if (this is Inet4Address) 32 else 128
