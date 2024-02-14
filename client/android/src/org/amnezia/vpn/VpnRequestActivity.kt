package org.amnezia.vpn

import android.app.KeyguardManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.VpnService
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResult
import androidx.activity.result.contract.ActivityResultContracts.StartActivityForResult
import androidx.core.content.ContextCompat
import androidx.core.content.getSystemService
import org.amnezia.vpn.util.Log

private const val TAG = "VpnRequestActivity"

class VpnRequestActivity : ComponentActivity() {

    private var userPresentReceiver: BroadcastReceiver? = null
    private val requestLauncher =
        registerForActivityResult(StartActivityForResult(), ::checkRequestResult)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(TAG, "Start request activity")
        val requestIntent = VpnService.prepare(applicationContext)
        if (requestIntent != null) {
            if (getSystemService<KeyguardManager>()!!.isKeyguardLocked) {
                userPresentReceiver = object : BroadcastReceiver() {
                    override fun onReceive(context: Context?, intent: Intent?) =
                        requestLauncher.launch(requestIntent)
                }
                registerReceiver(userPresentReceiver, IntentFilter(Intent.ACTION_USER_PRESENT))
            } else {
                requestLauncher.launch(requestIntent)
            }
            return
        } else {
            onPermissionGranted()
            finish()
        }
    }

    override fun onDestroy() {
        userPresentReceiver?.let {
            unregisterReceiver(it)
        }
        super.onDestroy()
    }

    private fun checkRequestResult(result: ActivityResult) {
        when (result.resultCode) {
            RESULT_OK -> onPermissionGranted()
            else -> Toast.makeText(this, "Vpn permission denied", Toast.LENGTH_LONG).show()
        }
        finish()
    }

    private fun onPermissionGranted() {
        Toast.makeText(this, "Vpn permission granted", Toast.LENGTH_LONG).show()
        Intent(applicationContext, AmneziaVpnService::class.java).apply {
            putExtra(AFTER_PERMISSION_CHECK, true)
        }.also {
            ContextCompat.startForegroundService(this, it)
        }
    }
}
