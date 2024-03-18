package org.amnezia.vpn.protocol.wireguard

import android.util.Base64
import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.util.net.InetEndpoint

private const val WIREGUARD_DEFAULT_MTU = 1280

open class WireguardConfig protected constructor(
    protocolConfigBuilder: ProtocolConfig.Builder,
    val endpoint: InetEndpoint,
    val persistentKeepalive: Int,
    val publicKeyHex: String,
    val preSharedKeyHex: String?,
    val privateKeyHex: String
) : ProtocolConfig(protocolConfigBuilder) {

    protected constructor(builder: Builder) : this(
        builder,
        builder.endpoint,
        builder.persistentKeepalive,
        builder.publicKeyHex,
        builder.preSharedKeyHex,
        builder.privateKeyHex
    )

    fun toWgUserspaceString(): String = with(StringBuilder()) {
        appendDeviceLine(this)
        appendLine("replace_peers=true")
        appendPeerLine(this)
        return this.toString()
    }

    open fun appendDeviceLine(sb: StringBuilder) = with(sb) {
        appendLine("private_key=$privateKeyHex")
    }

    open fun appendPeerLine(sb: StringBuilder) = with(sb) {
        appendLine("public_key=$publicKeyHex")
        routes.forEach { route ->
            appendLine("allowed_ip=$route")
        }
        appendLine("endpoint=$endpoint")
        if (persistentKeepalive != 0)
            appendLine("persistent_keepalive_interval=$persistentKeepalive")
        if (preSharedKeyHex != null)
            appendLine("preshared_key=$preSharedKeyHex")
    }

    open class Builder : ProtocolConfig.Builder(true) {
        internal lateinit var endpoint: InetEndpoint
            private set

        internal var persistentKeepalive: Int = 0
            private set

        internal lateinit var publicKeyHex: String
            private set

        internal var preSharedKeyHex: String? = null
            private set

        internal lateinit var privateKeyHex: String
            private set

        override var mtu: Int = WIREGUARD_DEFAULT_MTU

        fun setEndpoint(endpoint: InetEndpoint) = apply { this.endpoint = endpoint }

        fun setPersistentKeepalive(persistentKeepalive: Int) = apply { this.persistentKeepalive = persistentKeepalive }

        fun setPublicKeyHex(publicKeyHex: String) = apply { this.publicKeyHex = publicKeyHex }

        fun setPreSharedKeyHex(preSharedKeyHex: String) = apply { this.preSharedKeyHex = preSharedKeyHex }

        fun setPrivateKeyHex(privateKeyHex: String) = apply { this.privateKeyHex = privateKeyHex }

        override fun build(): WireguardConfig = configBuild().run { WireguardConfig(this@Builder) }
    }

    companion object {
        inline fun build(block: Builder.() -> Unit): WireguardConfig = Builder().apply(block).build()
    }
}

@OptIn(ExperimentalStdlibApi::class)
internal fun String.base64ToHex(): String = Base64.decode(this, Base64.DEFAULT).toHexString()
