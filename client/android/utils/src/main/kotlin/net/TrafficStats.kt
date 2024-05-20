package org.amnezia.vpn.util.net

import android.net.TrafficStats
import android.os.Build
import android.os.Process
import android.os.SystemClock
import kotlin.math.roundToLong

private const val BYTE = 1L
private const val KiB = BYTE shl 10
private const val MiB = KiB shl 10
private const val GiB = MiB shl 10
private const val TiB = GiB shl 10

class TrafficStats {

    private var lastTrafficData = TrafficData.ZERO
    private var lastTimestamp = 0L

    private val getTrafficDataCompat: () -> TrafficData =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val iface = "tun0"
            fun(): TrafficData {
                return TrafficData(TrafficStats.getRxBytes(iface), TrafficStats.getTxBytes(iface))
            }
        } else {
            val uid = Process.myUid()
            fun(): TrafficData {
                return TrafficData(TrafficStats.getUidRxBytes(uid), TrafficStats.getUidTxBytes(uid))
            }
        }

    fun reset() {
        lastTrafficData = getTrafficDataCompat()
        lastTimestamp = SystemClock.elapsedRealtime()
    }

    fun isSupported(): Boolean =
        lastTrafficData.rx != TrafficStats.UNSUPPORTED.toLong() && lastTrafficData.tx != TrafficStats.UNSUPPORTED.toLong()

    fun getSpeed(): TrafficData {
        val timestamp = SystemClock.elapsedRealtime()
        val elapsedSeconds = (timestamp - lastTimestamp) / 1000.0
        val trafficData = getTrafficDataCompat()
        val speed = trafficData.diff(lastTrafficData, elapsedSeconds)
        lastTrafficData = trafficData
        lastTimestamp = timestamp
        return speed
    }

    class TrafficData(val rx: Long, val tx: Long) {

        private var _rxString: String? = null
        val rxString: String
            get() {
                if (_rxString == null) _rxString = rx.speedToString()
                return _rxString ?: throw AssertionError("Set to null by another thread")
            }

        private var _txString: String? = null
        val txString: String
            get() {
                if (_txString == null) _txString = tx.speedToString()
                return _txString ?: throw AssertionError("Set to null by another thread")
            }

        fun diff(other: TrafficData, elapsedSeconds: Double): TrafficData {
            val rx = ((this.rx - other.rx) / elapsedSeconds).round()
            val tx = ((this.tx - other.tx) / elapsedSeconds).round()
            return if (rx == 0L && tx == 0L) ZERO else TrafficData(rx, tx)
        }

        private fun Double.round() = if (isNaN()) 0L else roundToLong()

        private fun Long.speedToString() =
            when {
                this < KiB -> formatSize(this, BYTE, "B/s")
                this < MiB -> formatSize(this, KiB, "KiB/s")
                this < GiB -> formatSize(this, MiB, "MiB/s")
                this < TiB -> formatSize(this, GiB, "GiB/s")
                else -> formatSize(this, TiB, "TiB/s")
            }

        private fun formatSize(bytes: Long, divider: Long, unit: String): String {
            val s = (bytes.toDouble() / divider * 100).roundToLong() / 100.0
            return "${s.toString().removeSuffix(".0")} $unit"
        }

        companion object {
            val ZERO: TrafficData = TrafficData(0L, 0L)
        }
    }
}
