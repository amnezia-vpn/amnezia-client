package com.wireguard.android.backend

// TODO: Refactor Amnezia wireguard project by changing the JNI method names
//  to move this object to 'org.amnezia.vpn.protocol.wireguard.backend' package
object GoBackend {
    external fun wgGetConfig(handle: Int): String?
    external fun wgGetSocketV4(handle: Int): Int
    external fun wgGetSocketV6(handle: Int): Int
    external fun wgTurnOff(handle: Int)
    external fun wgTurnOn(ifName: String, tunFd: Int, settings: String): Int
    external fun wgVersion(): String
}
