/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2017 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2017 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
 *                                                                             *
 *  This program is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by       *
 *  the Free Software Foundation, either version 3 of the License, or          *
 *  (at your option) any later version.                                        *
 *                                                                             *
 *  This program is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 *  GNU General Public License for more details.                               *
 *                                                                             *
 *  You should have received a copy of the GNU General Public License          *
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

package org.amnezia.vpn.shadowsocks.core

import android.app.KeyguardManager
import android.content.BroadcastReceiver
import android.content.Intent
import android.content.IntentFilter
import android.net.VpnService
import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.getSystemService

import org.amnezia.vpn.shadowsocks.core.preference.DataStore
import org.amnezia.vpn.shadowsocks.core.utils.Key
import org.amnezia.vpn.shadowsocks.core.utils.broadcastReceiver

class VpnRequestActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "VpnRequestActivity"
        private const val REQUEST_CONNECT = 1
    }

    private var receiver: BroadcastReceiver? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (DataStore.serviceMode != Key.modeVpn) {
            finish()
            return
        }
        if (getSystemService<KeyguardManager>()!!.isKeyguardLocked) {
            receiver = broadcastReceiver { _, _ -> request() }
            registerReceiver(receiver, IntentFilter(Intent.ACTION_USER_PRESENT))
        } else request()
    }

    private fun request() {
        val intent = VpnService.prepare(this)
        if (intent == null) onActivityResult(REQUEST_CONNECT, RESULT_OK, null)
        else startActivityForResult(intent, REQUEST_CONNECT)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (resultCode == RESULT_OK) Core.startService() else {
            Toast.makeText(this, R.string.vpn_permission_denied, Toast.LENGTH_LONG).show()
        }
        finish()
    }

    override fun onDestroy() {
        super.onDestroy()
        if (receiver != null) unregisterReceiver(receiver)
    }
}
