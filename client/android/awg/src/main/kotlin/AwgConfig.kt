package org.amnezia.vpn.protocol.awg

import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.wireguard.WireguardConfig

class AwgConfig private constructor(
    wireguardConfigBuilder: WireguardConfig.Builder,
    val jc: Int,
    val jmin: Int,
    val jmax: Int,
    val s1: Int,
    val s2: Int,
    val h1: Long,
    val h2: Long,
    val h3: Long,
    val h4: Long
) : WireguardConfig(wireguardConfigBuilder) {

    private constructor(builder: Builder) : this(
        builder,
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

    override fun appendDeviceLine(sb: StringBuilder) = with(sb) {
        super.appendDeviceLine(this)
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

    class Builder : WireguardConfig.Builder() {

        private var _jc: Int? = null
        internal var jc: Int
            get() = _jc ?: throw BadConfigException("AWG: parameter jc is undefined")
            private set(value) { _jc = value }

        private var _jmin: Int? = null
        internal var jmin: Int
            get() = _jmin ?: throw BadConfigException("AWG: parameter jmin is undefined")
            private set(value) { _jmin = value }

        private var _jmax: Int? = null
        internal var jmax: Int
            get() = _jmax ?: throw BadConfigException("AWG: parameter jmax is undefined")
            private set(value) { _jmax = value }

        private var _s1: Int? = null
        internal var s1: Int
            get() = _s1 ?: throw BadConfigException("AWG: parameter s1 is undefined")
            private set(value) { _s1 = value }

        private var _s2: Int? = null
        internal var s2: Int
            get() = _s2 ?: throw BadConfigException("AWG: parameter s2 is undefined")
            private set(value) { _s2 = value }

        private var _h1: Long? = null
        internal var h1: Long
            get() = _h1 ?: throw BadConfigException("AWG: parameter h1 is undefined")
            private set(value) { _h1 = value }

        private var _h2: Long? = null
        internal var h2: Long
            get() = _h2 ?: throw BadConfigException("AWG: parameter h2 is undefined")
            private set(value) { _h2 = value }

        private var _h3: Long? = null
        internal var h3: Long
            get() = _h3 ?: throw BadConfigException("AWG: parameter h3 is undefined")
            private set(value) { _h3 = value }

        private var _h4: Long? = null
        internal var h4: Long
            get() = _h4 ?: throw BadConfigException("AWG: parameter h4 is undefined")
            private set(value) { _h4 = value }

        fun setJc(jc: Int) = apply { this.jc = jc }
        fun setJmin(jmin: Int) = apply { this.jmin = jmin }
        fun setJmax(jmax: Int) = apply { this.jmax = jmax }
        fun setS1(s1: Int) = apply { this.s1 = s1 }
        fun setS2(s2: Int) = apply { this.s2 = s2 }
        fun setH1(h1: Long) = apply { this.h1 = h1 }
        fun setH2(h2: Long) = apply { this.h2 = h2 }
        fun setH3(h3: Long) = apply { this.h3 = h3 }
        fun setH4(h4: Long) = apply { this.h4 = h4 }

        override fun build(): AwgConfig = configBuild().run { AwgConfig(this@Builder) }
    }

    companion object {
        inline fun build(block: Builder.() -> Unit): AwgConfig = Builder().apply(block).build()
    }
}
