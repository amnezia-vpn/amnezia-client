package org.amnezia.vpn.protocol.xray

import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.util.net.InetNetwork

private const val XRAY_DEFAULT_MTU = 1500
private const val XRAY_DEFAULT_MAX_MEMORY: Long = 50 shl 20 // 50 MB

class XrayConfig protected constructor(
    protocolConfigBuilder: ProtocolConfig.Builder,
    val socksPort: Int,
    val maxMemory: Long,
) : ProtocolConfig(protocolConfigBuilder) {

    protected constructor(builder: Builder) : this(
        builder,
        builder.socksPort,
        builder.maxMemory
    )

    class Builder : ProtocolConfig.Builder(false) {
        internal var socksPort: Int = 0
            private set

        internal var maxMemory: Long = XRAY_DEFAULT_MAX_MEMORY
            private set

        override var mtu: Int = XRAY_DEFAULT_MTU

        fun setSocksPort(port: Int) = apply { socksPort = port }

        fun setMaxMemory(maxMemory: Long) = apply { this.maxMemory = maxMemory }

        override fun build(): XrayConfig = configBuild().run { XrayConfig(this@Builder) }
    }

    companion object {
        internal val DEFAULT_IPV4_ADDRESS: InetNetwork = InetNetwork("10.0.42.2", 30)

        inline fun build(block: Builder.() -> Unit): XrayConfig = Builder().apply(block).build()
    }
}
