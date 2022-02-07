package org.amnezia.vpn.qt;

import android.app.Activity;
import android.os.Bundle;

import org.amnezia.vpn.BuildConfig;

public class VPNApplication extends org.qtproject.qt5.android.bindings.QtApplication {

  private static VPNApplication instance;

  @Override
  public void onCreate() {
      super.onCreate();
      VPNApplication.instance = this;
  }

}
