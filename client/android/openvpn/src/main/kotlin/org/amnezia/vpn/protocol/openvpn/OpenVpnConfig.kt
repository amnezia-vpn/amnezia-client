package org.amnezia.vpn.protocol.openvpn

import org.amnezia.vpn.protocol.ProtocolConfig

private const val OPENVPN_DEFAULT_MTU = 1500

class OpenVpnConfig private constructor(
    protocolConfigBuilder: ProtocolConfig.Builder
) : ProtocolConfig(protocolConfigBuilder) {

    class Builder : ProtocolConfig.Builder(false) {
        override var mtu: Int = OPENVPN_DEFAULT_MTU

        override fun build(): OpenVpnConfig = configBuild().run { OpenVpnConfig(this@Builder) }
    }

    companion object {
        inline fun build(block: Builder.() -> Unit): OpenVpnConfig = Builder().apply(block).build()
    }
}
