package org.amnezia.vpn.qt

import android.Manifest
import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.os.IBinder
import android.util.Log
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import org.amnezia.vpn.VPNService
import org.amnezia.vpn.VPNServiceBinder
import org.qtproject.qt5.android.bindings.QtActivity
import java.io.BufferedReader

class VPNActivity : QtActivity() {
    private var configString: String? = null
    private var vpnServiceBinder: VPNServiceBinder? = null

    private val TAG = "VPNActivity"
    private val STORAGE_PERMISSION_CODE = 42

    override fun onNewIntent(newIntent: Intent) {
        if (isReadStorageAllowed()) {
            processIntent(newIntent)
        } else {
            intent = newIntent
            requestStoragePermission()
        }
    }

    override fun onResume() {
        super.onResume()
        if (intent.action == Intent.ACTION_VIEW) {
            if (isReadStorageAllowed()) {
                processIntent(intent)
            } else {
                requestStoragePermission()
            }
        }
    }

    override fun onPause() {
        if (vpnServiceBinder != null) {
            unbindService(connection)
        }
        super.onPause()
    }

    private fun isReadStorageAllowed(): Boolean {
        val permissionStatus = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
        return permissionStatus == PackageManager.PERMISSION_GRANTED
    }

    private fun requestStoragePermission() {
        ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE), STORAGE_PERMISSION_CODE)
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String?>, grantResults: IntArray) {
        if (requestCode == STORAGE_PERMISSION_CODE) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Log.d(TAG, "Storage read permission granted")
                processIntent(intent)
            } else {
                Toast.makeText(this, "Oops you just denied the permission", Toast.LENGTH_LONG).show()
            }
        }
    }

    private fun processIntent(intent: Intent) {
        // Get file URI from the incoming Intent
        intent.data?.also { fileUri ->
            try {
                val inputStream = contentResolver.openInputStream(fileUri)
                if (inputStream == null) {
                    Log.e(TAG, "Can not open: $fileUri")
                }
                configString = inputStream?.bufferedReader()?.use(BufferedReader::readText)
            } catch (e: Throwable) {
                Log.e(TAG, "Shared file opening error: ${e.message}")
            }
            doServiceBinding()
        }
        intent.data = null
    }

    var connection: ServiceConnection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, binder: IBinder) {
            vpnServiceBinder = (binder as VPNServiceBinder)
            if (configString != null) {
                vpnServiceBinder?.importConfig(configString!!)
                configString = null
            }
        }

        override fun onServiceDisconnected(className: ComponentName) {
            Log.d("ServiceConnection", "disconnected")
            vpnServiceBinder = null
        }
    }

    fun doServiceBinding() {
        val intent = Intent(this, VPNService::class.java)
        bindService(intent, connection, BIND_AUTO_CREATE)
    }

}
