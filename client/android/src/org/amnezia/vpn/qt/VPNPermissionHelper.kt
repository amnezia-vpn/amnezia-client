/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn.qt

import android.content.Context
import android.content.Intent

class VPNPermissionHelper : android.net.VpnService() {
    /**
     * This small service does nothing else then checking if the vpn permission
     * is present and prompting if not.
     */
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val intent = prepare(this.applicationContext)
        if (intent != null) {
            startActivityForResult(intent)
        }
        return START_NOT_STICKY
    }

    companion object {
        @JvmStatic
        fun startService(c: Context) {
            val appC = c.applicationContext
            appC.startService(Intent(appC, VPNPermissionHelper::class.java))
        }
    }

    /**
     * Fetches the Global QTAndroidActivity and calls startActivityForResult with the given intent
     * Is used to request the VPN-Permission, if not given.
     * Actually Implemented in src/platforms/android/AndroidController.cpp
     */
    external fun startActivityForResult(i: Intent)
}
