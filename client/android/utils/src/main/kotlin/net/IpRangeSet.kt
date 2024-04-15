package org.amnezia.vpn.util.net

class IpRangeSet {

    private val ranges = sortedSetOf<IpRange>()

    fun add(ipRange: IpRange) {
        val iterator = ranges.iterator()
        var rangeToAdd = ipRange
        run {
            while (iterator.hasNext()) {
                val curRange = iterator.next()
                if (rangeToAdd.end < curRange.start &&
                    !rangeToAdd.end.isMaxIp() &&
                    rangeToAdd.end.inc() != curRange.start) break
                (curRange + rangeToAdd)?.let { resultRange ->
                    if (resultRange == curRange) return@run
                    iterator.remove()
                    rangeToAdd = resultRange
                }
            }
            ranges += rangeToAdd
        }
    }

    fun remove(ipRange: IpRange) {
        val iterator = ranges.iterator()
        val splitRanges = mutableListOf<IpRange>()
        while (iterator.hasNext()) {
            val curRange = iterator.next()
            if (ipRange.end < curRange.start) break
            (curRange - ipRange)?.let { resultRanges ->
                iterator.remove()
                splitRanges += resultRanges
            }
        }
        ranges += splitRanges
    }

    fun subnets(): List<InetNetwork> = ranges.map(IpRange::subnets).flatten()

    override fun toString(): String = ranges.toString()
}
