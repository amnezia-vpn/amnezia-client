package org.amnezia.vpn.shadowsocks.core.plugin

import org.amnezia.vpn.shadowsocks.core.Core.app
import org.amnezia.vpn.shadowsocks.core.R

object NoPlugin : Plugin() {
    override val id: String get() = ""
    override val label: CharSequence get() = app.getText(R.string.plugin_disabled)
}
