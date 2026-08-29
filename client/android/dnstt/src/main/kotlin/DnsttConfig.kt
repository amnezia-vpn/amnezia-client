package org.amnezia.vpn.protocol.dnstt

import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.util.net.InetNetwork

// The TUN MTU is a normal interface MTU. It is unrelated to the dnstt payload
// MTU derived from the tunnel domain, which applies to the KCP layer inside the
// tunnel and is enforced by libdnstt itself.
private const val DNSTT_DEFAULT_MTU = 1500

class DnsttConfig protected constructor(
    protocolConfigBuilder: ProtocolConfig.Builder,
    val domain: String,
    val resolvers: String,
    val bootstrapIp: String,
    val publicKey: String
) : ProtocolConfig(protocolConfigBuilder) {

    protected constructor(builder: Builder) : this(
        builder,
        builder.domain,
        builder.resolvers,
        builder.bootstrapIp,
        builder.publicKey
    )

    class Builder : ProtocolConfig.Builder(false) {
        internal var domain: String = ""
            private set
        internal var resolvers: String = "https://1.1.1.1/dns-query"
            private set
        internal var bootstrapIp: String = ""
            private set
        internal var publicKey: String = ""
            private set

        override var mtu: Int = DNSTT_DEFAULT_MTU

        fun setDomain(domain: String) = apply { this.domain = domain }
        fun setResolvers(resolvers: String) = apply { this.resolvers = resolvers }
        fun setBootstrapIp(bootstrapIp: String) = apply { this.bootstrapIp = bootstrapIp }
        fun setPublicKey(publicKey: String) = apply { this.publicKey = publicKey }

        override fun build(): DnsttConfig = configBuild().run { DnsttConfig(this@Builder) }
    }

    companion object {
        internal val DEFAULT_IPV4_ADDRESS: InetNetwork = InetNetwork("10.0.42.2", 30)

        inline fun build(block: Builder.() -> Unit): DnsttConfig = Builder().apply(block).build()
    }
}
