package org.amnezia.vpn.qt

import android.content.res.Configuration
import org.amnezia.vpn.shadowsocks.core.Core
import org.amnezia.vpn.shadowsocks.core.VpnManager
import org.qtproject.qt.android.bindings.QtActivity
import org.qtproject.qt.android.bindings.QtApplication
import android.app.Application

class AmneziaApp: org.qtproject.qt.android.bindings.QtApplication() {

    override fun onCreate() {
        super.onCreate()
        Core.init(this, QtActivity::class)
        VpnManager.getInstance().init(this)
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        Core.updateNotificationChannels()
    }
}
