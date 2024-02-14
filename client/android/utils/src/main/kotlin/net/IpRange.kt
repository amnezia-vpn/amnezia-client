package org.amnezia.vpn.util.net

import java.net.InetAddress

class IpRange(private val start: IpAddress, private val end: IpAddress) : Comparable<IpRange> {

    init {
        if (start > end) throw IllegalArgumentException("Start IP: $start is greater then end IP: $end")
    }

    private constructor(addresses: Pair<IpAddress, IpAddress>) : this(addresses.first, addresses.second)

    constructor(inetAddress: InetAddress, mask: Int) : this(from(inetAddress, mask))

    constructor(address: String, mask: Int) : this(parseInetAddress(address), mask)

    constructor(inetNetwork: InetNetwork) : this(from(inetNetwork))

    private operator fun contains(other: IpRange): Boolean =
        (start <= other.start) && (end >= other.end)

    private fun isIntersect(other: IpRange): Boolean =
        (start <= other.end) && (end >= other.start)

    operator fun minus(other: IpRange): List<IpRange>? {
        if (this in other) return emptyList()
        if (!isIntersect(other)) return null
        val resultRanges = mutableListOf<IpRange>()
        if (start < other.start) resultRanges += IpRange(start, other.start.dec())
        if (end > other.end) resultRanges += IpRange(other.end.inc(), end)
        return resultRanges
    }

    fun subnets(): List<InetNetwork> {
        var currentIp = start
        var mask: Int
        val subnets = mutableListOf<InetNetwork>()
        while (currentIp <= end) {
            mask = getPossibleMaxMask(currentIp)
            var lastIp = getLastIpForMask(currentIp, mask)
            while (lastIp > end) {
                lastIp = getLastIpForMask(currentIp, ++mask)
            }
            subnets.add(InetNetwork(currentIp.toString(), mask))
            if (lastIp.isMaxIp()) break
            currentIp = lastIp.inc()
        }
        return subnets
    }

    private fun getPossibleMaxMask(ip: IpAddress): Int {
        var mask = ip.maxMask
        for (i in ip.lastIndex downTo 0) {
            val lastZeroBits = ip[i].countTrailingZeroBits()
            mask -= lastZeroBits
            if (lastZeroBits != 8) break
        }
        return mask
    }

    private fun getLastIpForMask(ip: IpAddress, mask: Int): IpAddress {
        var remainingBits = ip.maxMask - mask
        if (remainingBits == 0) return ip
        var i = ip.lastIndex
        val lastIp = ip.copy()
        while (remainingBits > 0 && i >= 0) {
            lastIp[i] =
                if (remainingBits > 8) {
                    lastIp[i] or 0xffu
                } else {
                    lastIp[i] or ((0xffu shl remainingBits).toUByte().inv())
                }
            remainingBits -= 8
            --i
        }
        return lastIp
    }

    override fun compareTo(other: IpRange): Int {
        val d = start.compareTo(other.start)
        return if (d == 0) end.compareTo(other.end) else d
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false
        other as IpRange
        return compareTo(other) == 0
    }

    override fun hashCode(): Int {
        var result = start.hashCode()
        result = 31 * result + end.hashCode()
        return result
    }

    override fun toString(): String {
        return "$start - $end"
    }

    companion object {
        private fun from(inetAddress: InetAddress, mask: Int): Pair<IpAddress, IpAddress> {
            val start = IpAddress(inetAddress)
            val end = IpAddress(inetAddress)
            val lastByte = mask / 8
            if (lastByte < start.size) {
                val byteMask = (0xffu shl (8 - mask % 8)).toUByte()
                start[lastByte] = start[lastByte].and(byteMask)
                end[lastByte] = end[lastByte].or(byteMask.inv())
                start.fill(0u, lastByte + 1)
                end.fill(0xffu, lastByte + 1)
            }
            return Pair(start, end)
        }

        private fun from(inetNetwork: InetNetwork): Pair<IpAddress, IpAddress> =
            from(inetNetwork.address, inetNetwork.mask)
    }
}
