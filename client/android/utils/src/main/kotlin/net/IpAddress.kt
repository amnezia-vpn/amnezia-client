package org.amnezia.vpn.util.net

import java.net.InetAddress

@OptIn(ExperimentalUnsignedTypes::class)
class IpAddress private constructor(private val address: UByteArray) : Comparable<IpAddress> {

    val size: Int = address.size
    val lastIndex: Int = address.lastIndex
    val maxMask: Int = size * 8

    constructor(inetAddress: InetAddress) : this(inetAddress.address.asUByteArray())

    constructor(ipAddress: String) : this(parseInetAddress(ipAddress))

    operator fun get(i: Int): UByte = address[i]

    operator fun set(i: Int, b: UByte) { address[i] = b }

    fun fill(value: UByte, fromByte: Int) = address.fill(value, fromByte)

    fun copy(): IpAddress = IpAddress(address.copyOf())

    fun inc(): IpAddress {
        if (address.all { it == 0xffu.toUByte() }) {
            throw RuntimeException("IP address overflow")
        }
        val copy = copy()
        for (i in copy.lastIndex downTo 0) {
            if (++copy[i] != 0u.toUByte()) break
        }
        return copy
    }

    fun dec(): IpAddress {
        if (address.all { it == 0u.toUByte() }) {
            throw RuntimeException("IP address overflow")
        }
        val copy = copy()
        for (i in copy.lastIndex downTo 0) {
            if (--copy[i] != 0xffu.toUByte()) break
        }
        return copy
    }

    fun isMaxIp(): Boolean = address.all { it == 0xffu.toUByte() }

    override fun compareTo(other: IpAddress): Int {
        if (size != other.size) return size - other.size
        for (i in address.indices) {
            val d = (address[i] - other.address[i]).toInt()
            if (d != 0) return d
        }
        return 0
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false
        other as IpAddress
        return compareTo(other) == 0
    }

    override fun hashCode(): Int {
        return address.hashCode()
    }

    override fun toString(): String {
        if (size > 4) return toIpv6String()
        return address.joinToString(".")
    }

    @OptIn(ExperimentalStdlibApi::class)
    private fun toIpv6String(): String {
        val sb = StringBuilder()
        var i = 0
        while (i < size) {
            sb.append(address[i++].toHexString())
            sb.append(address[i++].toHexString())
            sb.append(':')
        }
        sb.deleteAt(sb.lastIndex)
        return sb.toString()
    }
}
