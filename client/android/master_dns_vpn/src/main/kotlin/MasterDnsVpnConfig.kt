package org.amnezia.vpn.protocol.masterdnsvpn

import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.util.net.InetNetwork

private const val MASTER_DNS_VPN_DEFAULT_MTU = 1500

/**
 * Per-connection configuration for the MasterDnsVPN protocol on Android.
 *
 * Built from the JSON received from the desktop/IPC layer via the
 * [Builder]. The `socksPort` is filled in at runtime after the native
 * engine binds its listener (it cannot be predicted at config time
 * because the engine asks the OS for an ephemeral port).
 */
class MasterDnsVpnConfig protected constructor(
    protocolConfigBuilder: ProtocolConfig.Builder,
    val socksPort: Int,
) : ProtocolConfig(protocolConfigBuilder) {

    protected constructor(builder: Builder) : this(
        builder,
        builder.socksPort,
    )

    /**
     * Mutable accumulator for [MasterDnsVpnConfig] fields. Inherits the
     * generic protocol-config fields (addresses, routes, DNS, MTU,
     * split-tunnel rules) from [ProtocolConfig.Builder] and adds the one
     * MasterDnsVPN-specific field, [socksPort].
     */
    class Builder : ProtocolConfig.Builder(false) {
        internal var socksPort: Int = 0
            private set

        override var mtu: Int = MASTER_DNS_VPN_DEFAULT_MTU

        /** Set the loopback SOCKS5 listener port published by the engine. */
        fun setSocksPort(port: Int) = apply { socksPort = port }

        override fun build(): MasterDnsVpnConfig =
            configBuild().run { MasterDnsVpnConfig(this@Builder) }
    }

    /**
     * Static defaults and the DSL-style [build] entry point.
     */
    companion object {
        // /30 private subnet for the TUN interface; matches the /30 trick
        // XrayConfig uses to give us a gateway/.1 + TUN local/.2 pair that
        // doesn't collide with typical LAN ranges.
        internal val DEFAULT_IPV4_ADDRESS: InetNetwork = InetNetwork("10.0.43.2", 30)

        /** Apply [block] to a fresh [Builder] and produce the config. */
        inline fun build(block: Builder.() -> Unit): MasterDnsVpnConfig =
            Builder().apply(block).build()
    }
}
