package org.amnezia.vpn.protocol.hysteria2

import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.util.net.InetNetwork

private const val HYSTERIA2_DEFAULT_MTU = 1500

class Hysteria2Config protected constructor(
    protocolConfigBuilder: ProtocolConfig.Builder,
    val socksPort: Int,
) : ProtocolConfig(protocolConfigBuilder) {

    protected constructor(builder: Builder) : this(
        builder,
        builder.socksPort,
    )

    class Builder : ProtocolConfig.Builder(false) {
        internal var socksPort: Int = 10808
            private set

        override var mtu: Int = HYSTERIA2_DEFAULT_MTU

        fun setSocksPort(port: Int) = apply { socksPort = port }

        override fun build(): Hysteria2Config = configBuild().run { Hysteria2Config(this@Builder) }
    }

    companion object {
        internal val DEFAULT_IPV4_ADDRESS: InetNetwork = InetNetwork("10.33.0.2", 16)

        inline fun build(block: Builder.() -> Unit): Hysteria2Config = Builder().apply(block).build()
    }
}
