package org.amnezia.vpn

import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.awg.Awg
import org.amnezia.vpn.protocol.cloak.Cloak
import org.amnezia.vpn.protocol.openvpn.OpenVpn
import org.amnezia.vpn.protocol.wireguard.Wireguard
import org.amnezia.vpn.protocol.xray.Xray

enum class VpnProto(
    val label: String,
    val processName: String,
    val serviceClass: Class<out AmneziaVpnService>
) {
    WIREGUARD(
        "WireGuard",
        "org.amnezia.vpn:amneziaAwgService",
        AwgService::class.java
    ) {
        override fun createProtocol(): Protocol = Wireguard()
    },

    AWG(
        "AmneziaWG",
        "org.amnezia.vpn:amneziaAwgService",
        AwgService::class.java
    ) {
        override fun createProtocol(): Protocol = Awg()
    },

    OPENVPN(
        "OpenVPN",
        "org.amnezia.vpn:amneziaOpenVpnService",
        OpenVpnService::class.java
    ) {
        override fun createProtocol(): Protocol = OpenVpn()
    },

    CLOAK(
        "Cloak",
        "org.amnezia.vpn:amneziaOpenVpnService",
        OpenVpnService::class.java
    ) {
        override fun createProtocol(): Protocol = Cloak()
    },

    XRAY(
        "XRay",
        "org.amnezia.vpn:amneziaXrayService",
        XrayService::class.java
    ) {
        override fun createProtocol(): Protocol = Xray.instance
    },

    SSXRAY(
        "SSXRay",
        "org.amnezia.vpn:amneziaXrayService",
        XrayService::class.java
    ) {
        override fun createProtocol(): Protocol = Xray.instance
    };

    private var _protocol: Protocol? = null
    val protocol: Protocol
        get() {
            if (_protocol == null) _protocol = createProtocol()
            return _protocol ?: throw AssertionError("Set to null by another thread")
        }

    protected abstract fun createProtocol(): Protocol

    companion object {
        fun get(protocolName: String): VpnProto = VpnProto.valueOf(protocolName.uppercase())
    }
}