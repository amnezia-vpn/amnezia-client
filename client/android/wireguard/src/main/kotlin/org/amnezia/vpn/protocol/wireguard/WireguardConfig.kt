package org.amnezia.vpn.protocol.wireguard

import android.util.Base64
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.ProtocolConfig
import org.amnezia.vpn.util.net.InetEndpoint

private const val WIREGUARD_DEFAULT_MTU = 1280

open class WireguardConfig protected constructor(
    protocolConfigBuilder: ProtocolConfig.Builder,
    val endpoint: InetEndpoint,
    val persistentKeepalive: Int,
    val publicKeyHex: String,
    val preSharedKeyHex: String?,
    val privateKeyHex: String,
    val useProtocolExtension: Boolean,
    val jc: Int?,
    val jmin: Int?,
    val jmax: Int?,
    val s1: Int?,
    val s2: Int?,
    val h1: Long?,
    val h2: Long?,
    val h3: Long?,
    val h4: Long?
) : ProtocolConfig(protocolConfigBuilder) {

    protected constructor(builder: Builder) : this(
        builder,
        builder.endpoint,
        builder.persistentKeepalive,
        builder.publicKeyHex,
        builder.preSharedKeyHex,
        builder.privateKeyHex,
        builder.useProtocolExtension,
        builder.jc,
        builder.jmin,
        builder.jmax,
        builder.s1,
        builder.s2,
        builder.h1,
        builder.h2,
        builder.h3,
        builder.h4
    )

    fun toWgUserspaceString(): String = with(StringBuilder()) {
        appendDeviceLine(this)
        appendLine("replace_peers=true")
        appendPeerLine(this)
        return this.toString()
    }

    open fun appendDeviceLine(sb: StringBuilder) = with(sb) {
        appendLine("private_key=$privateKeyHex")
        if (useProtocolExtension) {
            validateProtocolExtensionParameters()
            appendLine("jc=$jc")
            appendLine("jmin=$jmin")
            appendLine("jmax=$jmax")
            appendLine("s1=$s1")
            appendLine("s2=$s2")
            appendLine("h1=$h1")
            appendLine("h2=$h2")
            appendLine("h3=$h3")
            appendLine("h4=$h4")
        }
    }

    private fun validateProtocolExtensionParameters() {
        if (jc == null) throw BadConfigException("Parameter jc is undefined")
        if (jmin == null) throw BadConfigException("Parameter jmin is undefined")
        if (jmax == null) throw BadConfigException("Parameter jmax is undefined")
        if (s1 == null) throw BadConfigException("Parameter s1 is undefined")
        if (s2 == null) throw BadConfigException("Parameter s2 is undefined")
        if (h1 == null) throw BadConfigException("Parameter h1 is undefined")
        if (h2 == null) throw BadConfigException("Parameter h2 is undefined")
        if (h3 == null) throw BadConfigException("Parameter h3 is undefined")
        if (h4 == null) throw BadConfigException("Parameter h4 is undefined")
    }

    open fun appendPeerLine(sb: StringBuilder) = with(sb) {
        appendLine("public_key=$publicKeyHex")
        routes.filter { it.include }.forEach { route ->
            appendLine("allowed_ip=${route.inetNetwork}")
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

        internal var useProtocolExtension: Boolean = false

        internal var jc: Int? = null
        internal var jmin: Int? = null
        internal var jmax: Int? = null
        internal var s1: Int? = null
        internal var s2: Int? = null
        internal var h1: Long? = null
        internal var h2: Long? = null
        internal var h3: Long? = null
        internal var h4: Long? = null

        fun setEndpoint(endpoint: InetEndpoint) = apply { this.endpoint = endpoint }

        fun setPersistentKeepalive(persistentKeepalive: Int) = apply { this.persistentKeepalive = persistentKeepalive }

        fun setPublicKeyHex(publicKeyHex: String) = apply { this.publicKeyHex = publicKeyHex }

        fun setPreSharedKeyHex(preSharedKeyHex: String) = apply { this.preSharedKeyHex = preSharedKeyHex }

        fun setPrivateKeyHex(privateKeyHex: String) = apply { this.privateKeyHex = privateKeyHex }

        fun setUseProtocolExtension(useProtocolExtension: Boolean) = apply { this.useProtocolExtension = useProtocolExtension }

        fun setJc(jc: Int) = apply { this.jc = jc }
        fun setJmin(jmin: Int) = apply { this.jmin = jmin }
        fun setJmax(jmax: Int) = apply { this.jmax = jmax }
        fun setS1(s1: Int) = apply { this.s1 = s1 }
        fun setS2(s2: Int) = apply { this.s2 = s2 }
        fun setH1(h1: Long) = apply { this.h1 = h1 }
        fun setH2(h2: Long) = apply { this.h2 = h2 }
        fun setH3(h3: Long) = apply { this.h3 = h3 }
        fun setH4(h4: Long) = apply { this.h4 = h4 }

        override fun build(): WireguardConfig = configBuild().run { WireguardConfig(this@Builder) }
    }

    companion object {
        inline fun build(block: Builder.() -> Unit): WireguardConfig = Builder().apply(block).build()
    }
}

@OptIn(ExperimentalStdlibApi::class)
internal fun String.base64ToHex(): String = Base64.decode(this, Base64.DEFAULT).toHexString()
