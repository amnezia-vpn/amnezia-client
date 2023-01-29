package org.amnezia.vpn.qt;

import android.content.res.Configuration;
import androidx.annotation.NonNull;
import org.amnezia.vpn.shadowsocks.core.Core;
import org.amnezia.vpn.shadowsocks.core.VpnManager;

public class VPNApplication extends org.qtproject.qt.android.bindings.QtApplication {
    private static VPNApplication instance;

    @Override
    public void onCreate() {
        super.onCreate();
        VPNApplication.instance = this;
//        Core.INSTANCE.init(this, VPNActivity.class);
//        VpnManager.Companion.getInstance().init(this);
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
//        Core.INSTANCE.updateNotificationChannels();
    }
}
