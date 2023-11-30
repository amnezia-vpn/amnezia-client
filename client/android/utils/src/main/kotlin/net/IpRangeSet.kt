package org.amnezia.vpn.util.net

class IpRangeSet(ipRange: IpRange = IpRange("0.0.0.0", 0)) {

    private val ranges = sortedSetOf(ipRange)

    fun remove(ipRange: IpRange) {
        val iterator = ranges.iterator()
        val splitRanges = mutableListOf<IpRange>()
        while (iterator.hasNext()) {
            val range = iterator.next()
            (range - ipRange)?.let { resultRanges ->
                iterator.remove()
                splitRanges += resultRanges
            }
        }
        ranges += splitRanges
    }

    fun subnets(): List<InetNetwork> =
        ranges.map(IpRange::subnets).flatten()

    override fun toString(): String {
        return ranges.toString()
    }
}
