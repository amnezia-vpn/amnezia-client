package org.amnezia.vpn.qt

import android.Manifest
import android.content.ComponentName
import android.content.ContentResolver
import android.content.Intent
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import android.os.IBinder
import android.provider.MediaStore
import android.util.Log
import android.view.KeyEvent
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import org.amnezia.vpn.VPNService
import org.amnezia.vpn.VPNServiceBinder
import org.qtproject.qt5.android.bindings.QtActivity
import java.io.*


class VPNActivity : QtActivity() {
    private var configString: String? = null
    private var vpnServiceBinder: VPNServiceBinder? = null

    private val TAG = "VPNActivity"
    private val STORAGE_PERMISSION_CODE = 42


    override fun onCreate(savedInstanceState: Bundle?) {
        val intent = intent
        val action = intent.action
        configString = processIntent(intent, action)
        super.onCreate(savedInstanceState)
    }

    override fun onNewIntent(newIntent: Intent) {
        intent = newIntent
        if (isReadStorageAllowed()) {
            val intent = intent
            val action = intent.action
            configString = processIntent(intent, action)
        } else {
            requestStoragePermission()
        }
        super.onNewIntent(intent)
    }

    override fun onResume() {
        super.onResume()

        if (configString != null) {
            doServiceBinding()
        }
    }

    override fun onPause() {
        if (vpnServiceBinder != null) {
            unbindService(connection)
        }
        super.onPause()
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.repeatCount == 0) {
            onBackPressed()
            return true
        }
        return super.onKeyDown(keyCode, event)
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

                if (configString != null) {
                    doServiceBinding()
                }
            } else {
                Toast.makeText(this, "Oops you just denied the permission", Toast.LENGTH_LONG).show()
            }
        }
    }

    private fun doServiceBinding() {
        try {
            val intent = Intent(this, VPNService::class.java)
            bindService(intent, connection, BIND_AUTO_CREATE)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun getContentName(resolver: ContentResolver?, uri: Uri?): String? {
        val cursor = resolver!!.query(uri!!, null, null, null, null)
        cursor!!.moveToFirst()
        val nameIndex = cursor.getColumnIndex(MediaStore.MediaColumns.DISPLAY_NAME)
        return if (nameIndex >= 0) {
            cursor.getString(nameIndex)
        } else {
            null
        }
    }


    private fun processIntent(intent: Intent, action: String?): String? {
        if (action!!.compareTo(Intent.ACTION_VIEW) == 0) {
            val scheme = intent.scheme
            val resolver = contentResolver
            if (scheme!!.compareTo(ContentResolver.SCHEME_CONTENT) == 0) {
                val uri = intent.data
                val name: String? = getContentName(resolver, uri)
                Log.e(TAG, "Content intent detected: " + action + " : " + intent.dataString + " : " + intent.type + " : " + name)
                val input = resolver.openInputStream(uri!!)
                return input?.bufferedReader()?.use(BufferedReader::readText)
            } else if (scheme.compareTo(ContentResolver.SCHEME_FILE) == 0) {
                val uri = intent.data
                val name = uri!!.lastPathSegment
                Log.e(TAG, "File intent detected: " + action + " : " + intent.dataString + " : " + intent.type + " : " + name)
                val input = resolver.openInputStream(uri)
                return input?.bufferedReader()?.use(BufferedReader::readText)
            }
        }
        return null
    }

    private var connection: ServiceConnection = object : ServiceConnection {
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

}
