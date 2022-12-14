package org.amnezia.vpn.qt;

import android.Manifest
import android.content.ComponentName
import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.net.Uri
import android.os.*
import android.provider.MediaStore
import android.util.Log
import android.view.KeyEvent
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import org.amnezia.vpn.VPNService
import org.amnezia.vpn.VPNServiceBinder
import org.amnezia.vpn.IMPORT_COMMAND_CODE
import org.amnezia.vpn.IMPORT_ACTION_CODE
import org.amnezia.vpn.IMPORT_CONFIG_KEY
import org.qtproject.qt5.android.bindings.QtActivity
import java.io.*

class VPNActivity : org.qtproject.qt5.android.bindings.QtActivity() {

    private var configString: String? = null
    private var vpnServiceBinder: Messenger? = null
    private var isBound = false

    private val TAG = "VPNActivity"
    private val STORAGE_PERMISSION_CODE = 42


    override fun onCreate(savedInstanceState: Bundle?) {
        val newIntent = intent
        val newIntentAction = newIntent.action

        if (newIntent != null && newIntentAction != null) {
            configString = processIntent(newIntent, newIntentAction)
        }

        super.onCreate(savedInstanceState)
    }

    override fun onNewIntent(newIntent: Intent) {
        intent = newIntent

        val newIntentAction = newIntent.action

        if (newIntent != null && newIntentAction != null && newIntentAction != Intent.ACTION_MAIN) {
            if (isReadStorageAllowed()) {
                configString = processIntent(newIntent, newIntentAction)
            } else {
                requestStoragePermission()
            }
        }

        super.onNewIntent(intent)
    }

    override fun onResume() {
        super.onResume()

        if (configString != null && !isBound) {
            bindVpnService()
        }
    }

    override fun onPause() {
        if (vpnServiceBinder != null && isBound) {
            unbindService(connection)
            isBound = false
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

                if (configString != null) {
                    bindVpnService()
                }
            } else {
                Toast.makeText(this, "Oops you just denied the permission", Toast.LENGTH_LONG).show()
            }
        }
    }

    private fun bindVpnService() {
        try {
            val intent = Intent(this, VPNService::class.java)
            intent.action = IMPORT_ACTION_CODE

            bindService(intent, connection, Context.BIND_AUTO_CREATE)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun processIntent(intent: Intent, action: String): String? {
        val scheme = intent.scheme

        if (scheme == null) {
            return null
        }

        if (action.compareTo(Intent.ACTION_VIEW) == 0) {
            val resolver = contentResolver

            if (scheme.compareTo(ContentResolver.SCHEME_CONTENT) == 0) {
                val uri = intent.data
                val name: String? = getContentName(resolver, uri)

                Log.d(TAG, "Content intent detected: " + action + " : " + intent.dataString + " : " + intent.type + " : " + name)

                val input = resolver.openInputStream(uri!!)

                return input?.bufferedReader()?.use(BufferedReader::readText)
            } else if (scheme.compareTo(ContentResolver.SCHEME_FILE) == 0) {
                val uri = intent.data
                val name = uri!!.lastPathSegment

                Log.d(TAG, "File intent detected: " + action + " : " + intent.dataString + " : " + intent.type + " : " + name)

                val input = resolver.openInputStream(uri)

                return input?.bufferedReader()?.use(BufferedReader::readText)
            }
        }

        return null
    }

    private fun getContentName(resolver: ContentResolver?, uri: Uri?): String? {
        val cursor = resolver!!.query(uri!!, null, null, null, null)

        cursor.use {
            cursor!!.moveToFirst()
            val nameIndex = cursor.getColumnIndex(MediaStore.MediaColumns.DISPLAY_NAME)
            return if (nameIndex >= 0) {
                return cursor.getString(nameIndex)
            } else {
                null
            }
        }
    }

    private var connection: ServiceConnection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, binder: IBinder) {
            vpnServiceBinder = Messenger(binder)

            if (configString != null) {
                val msg: Message = Message.obtain(null, IMPORT_COMMAND_CODE, 0, 0)
                val bundle = Bundle()
                bundle.putString(IMPORT_CONFIG_KEY, configString!!)
                msg.data = bundle

                try {
                    vpnServiceBinder?.send(msg)
                } catch (e: RemoteException) {
                    e.printStackTrace()
                }

                configString = null
            }

            isBound = true
        }

        override fun onServiceDisconnected(className: ComponentName) {
            vpnServiceBinder = null
            isBound = false
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.repeatCount == 0) {
            onBackPressed()
            return true
        }
        return super.onKeyDown(keyCode, event)
    }
}
