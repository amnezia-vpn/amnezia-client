package org.amnezia.awg

object GoBackend {
    external fun awgGetConfig(handle: Int): String?
    external fun awgGetSocketV4(handle: Int): Int
    external fun awgGetSocketV6(handle: Int): Int
    external fun awgTurnOff(handle: Int)
    external fun awgTurnOn(ifName: String, tunFd: Int, settings: String): Int
    external fun awgVersion(): String
    external fun awgSetUidFilter(filter: UidFilter?): Int

    // Called from native code, once per new outbound flow, on a thread the JVM did not start.
    fun interface UidFilter {
        fun allow(network: String, srcIp: String, srcPort: Int, dstIp: String, dstPort: Int): Boolean
    }
}
