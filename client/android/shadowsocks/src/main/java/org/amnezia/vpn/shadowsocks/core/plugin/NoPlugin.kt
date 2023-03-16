package org.amnezia.vpn.shadowsocks.plugin

import org.amnezia.vpn.shadowsocks.core.Core.app

object NoPlugin : Plugin() {
    override val id: String get() = ""
    override val label: CharSequence get() = app.getText(org.amnezia.vpn.shadowsocks.core.R.string.plugin_disabled)
}
