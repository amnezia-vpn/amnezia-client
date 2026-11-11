package org.amnezia.vpn

import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.VPN_SESSION_NAME
import org.amnezia.vpn.protocol.xtun.Xtun

enum class VpnProto(
    val label: String,
    val processName: String,
    val serviceClass: Class<out AmneziaVpnService>
) {
    XTUN(
        VPN_SESSION_NAME,
        "org.amnezia.vpn:amneziaVpnService",
        XtunService::class.java
    ) {
        override fun createProtocol(): Protocol = Xtun()
    };

    private var _protocol: Protocol? = null
    val protocol: Protocol
        get() {
            if (_protocol == null) _protocol = createProtocol()
            return _protocol ?: throw AssertionError("Set to null by another thread")
        }

    protected abstract fun createProtocol(): Protocol
}
