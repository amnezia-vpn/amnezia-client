package org.amnezia.vpn;

import android.content.Context;
import android.app.KeyguardManager;
import android.content.Intent;
import org.qtproject.qt.android.bindings.QtActivity;


import static android.content.Context.KEYGUARD_SERVICE;

public class AuthHelper extends QtActivity {

   static final String TAG = "AuthHelper";
  
   public static Intent getAuthIntent(Context context) {
   	KeyguardManager mKeyguardManager = (KeyguardManager)context.getSystemService(KEYGUARD_SERVICE);
   	if (mKeyguardManager.isDeviceSecure()) {
                return mKeyguardManager.createConfirmDeviceCredentialIntent(null, null);
        } else {
        	return null;
        }     
   }
   
}
