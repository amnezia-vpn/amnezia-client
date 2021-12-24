package com.github.shadowsocks.plugin

import com.github.shadowsocks.Core.app

object NoPlugin : Plugin() {
    override val id: String get() = ""
    override val label: CharSequence get() = app.getText(org.amnezia.vpn.R.string.plugin_disabled)
}
