/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn.qt

import android.os.Binder
import android.os.Parcel
import android.util.Log

const val permissionRequired = 6

class VPNClientBinder() : Binder() {

    override fun onTransact(code: Int, data: Parcel, reply: Parcel?, flags: Int): Boolean {
        if (code == permissionRequired) {
            VPNActivity.getInstance().onPermissionRequest(code, data)
            return true
        }

        val buffer = data.createByteArray()
        val stringData = buffer?.let { String(it) }
        VPNActivity.getInstance().onServiceMessage(code, stringData)

        return true
    }
}
