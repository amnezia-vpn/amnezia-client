package org.amnezia.awg

object GoBackend {
    external fun awgGetConfig(handle: Int): String?
    external fun awgGetSocketV4(handle: Int): Int
    external fun awgGetSocketV6(handle: Int): Int
    external fun awgTurnOff(handle: Int)
    external fun awgTurnOn(ifName: String, tunFd: Int, settings: String): Int
    external fun awgVersion(): String
}
