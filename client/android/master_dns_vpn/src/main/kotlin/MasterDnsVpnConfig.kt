package org.amnezia.vpn.protocol.masterdnsvpn

import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.util.net.InetNetwork

private const val MASTER_DNS_VPN_DEFAULT_MTU = 1500

class MasterDnsVpnConfig protected constructor(
    protocolConfigBuilder: ProtocolConfig.Builder,
    val socksPort: Int,
) : ProtocolConfig(protocolConfigBuilder) {

    protected constructor(builder: Builder) : this(
        builder,
        builder.socksPort,
    )

    class Builder : ProtocolConfig.Builder(false) {
        internal var socksPort: Int = 0
            private set

        override var mtu: Int = MASTER_DNS_VPN_DEFAULT_MTU

        fun setSocksPort(port: Int) = apply { socksPort = port }

        override fun build(): MasterDnsVpnConfig =
            configBuild().run { MasterDnsVpnConfig(this@Builder) }
    }

    companion object {
        // /30 private subnet for the TUN interface; matches the /30 trick
        // XrayConfig uses to give us a gateway/.1 + TUN local/.2 pair that
        // doesn't collide with typical LAN ranges.
        internal val DEFAULT_IPV4_ADDRESS: InetNetwork = InetNetwork("10.0.43.2", 30)

        inline fun build(block: Builder.() -> Unit): MasterDnsVpnConfig =
            Builder().apply(block).build()
    }
}
