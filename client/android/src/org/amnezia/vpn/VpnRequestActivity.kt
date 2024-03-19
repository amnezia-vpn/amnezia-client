package org.amnezia.vpn

import android.app.AlertDialog
import android.app.KeyguardManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.res.Configuration.UI_MODE_NIGHT_MASK
import android.content.res.Configuration.UI_MODE_NIGHT_YES
import android.net.VpnService
import android.os.Bundle
import android.provider.Settings
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResult
import androidx.activity.result.contract.ActivityResultContracts.StartActivityForResult
import androidx.core.content.ContextCompat
import androidx.core.content.getSystemService
import org.amnezia.vpn.util.Log

private const val TAG = "VpnRequestActivity"

class VpnRequestActivity : ComponentActivity() {

    // used to detect always-on vpn while checking vpn permissions
    private var lastPauseTime = -1L
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

    override fun onPause() {
        super.onPause()
        lastPauseTime = System.currentTimeMillis()
    }

    override fun onDestroy() {
        userPresentReceiver?.let {
            unregisterReceiver(it)
        }
        super.onDestroy()
    }

    private fun checkRequestResult(result: ActivityResult) {
        when (val resultCode = result.resultCode) {
            RESULT_OK -> {
                onPermissionGranted()
                finish()
            }

            else -> {
                if (resultCode == RESULT_CANCELED && System.currentTimeMillis() - lastPauseTime < 200) {
                    Log.w(TAG, "Another always-on VPN is active, vpn permission auto-denied")
                    showVpnAlwaysOnErrorDialog()
                } else {
                    Log.w(TAG, "Vpn permission denied, resultCode: $resultCode")
                    Toast.makeText(this, resources.getText(R.string.vpnDenied), Toast.LENGTH_LONG).show()
                    finish()
                }
            }
        }
    }

    private fun onPermissionGranted() {
        Toast.makeText(this, resources.getString(R.string.vpnGranted), Toast.LENGTH_LONG).show()
        Intent(applicationContext, AmneziaVpnService::class.java).apply {
            putExtra(AFTER_PERMISSION_CHECK, true)
        }.also {
            ContextCompat.startForegroundService(this, it)
        }
    }

    private fun showVpnAlwaysOnErrorDialog() {
        AlertDialog.Builder(this, getDialogTheme())
            .setTitle(R.string.vpnSetupFailed)
            .setMessage(R.string.vpnSetupFailedMessage)
            .setNegativeButton(R.string.cancel) { _, _ -> }
            .setPositiveButton(R.string.openVpnSettings) { _, _ ->
                startActivity(Intent(Settings.ACTION_VPN_SETTINGS))
            }
            .setOnDismissListener { finish() }
            .show()
    }

    private fun getDialogTheme(): Int =
        if (resources.configuration.uiMode and UI_MODE_NIGHT_MASK == UI_MODE_NIGHT_YES)
            android.R.style.Theme_DeviceDefault_Dialog_Alert
        else
            android.R.style.Theme_DeviceDefault_Light_Dialog_Alert
}
