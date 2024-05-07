package org.amnezia.vpn.util.net

import java.net.InetAddress

@OptIn(ExperimentalUnsignedTypes::class)
internal class IpAddress private constructor(private val address: UByteArray) : Comparable<IpAddress> {

    val size: Int = address.size
    val lastIndex: Int = address.lastIndex
    val maxMask: Int = size * 8

    @OptIn(ExperimentalStdlibApi::class)
    val hexFormat: HexFormat by lazy {
        HexFormat { number.removeLeadingZeros = true }
    }

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

    fun isMinIp(): Boolean = address.all { it == 0x00u.toUByte() }

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
        var block: Int
        while (i < size) {
            block = address[i++].toInt() shl 8
            block += address[i++].toInt()
            sb.append(block.toHexString(hexFormat))
            sb.append(':')
        }
        sb.deleteAt(sb.lastIndex)
        return convertIpv6ToCanonicalForm(sb.toString())
    }
}
