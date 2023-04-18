/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn.qt;

import android.Manifest
import android.content.ClipData
import android.content.ClipboardManager
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
import org.qtproject.qt.android.bindings.QtActivity
import java.io.*

class VPNActivity : org.qtproject.qt.android.bindings.QtActivity() {

    private var configString: String? = null
    private var vpnServiceBinder: IBinder? = null
    private var isBound = false
        set(value) {
            field = value

            if (value && configString != null) {
                sendImportConfigCommand()
            }
        }

    private val TAG = "VPNActivity"

    private val CAMERA_ACTION_CODE = 101
    private val CREATE_FILE_ACTION_CODE = 102

    private var tmpFileContentToSave: String = ""

    private val delayedCommands: ArrayList<Pair<Int, String>> = ArrayList()

    companion object {
        private lateinit var instance: VPNActivity

        @JvmStatic fun getInstance(): VPNActivity {
            return instance
        }

        @JvmStatic fun connectService() {
            VPNActivity.getInstance().initServiceConnection()
        }

        @JvmStatic fun startQrCodeReader() {
            VPNActivity.getInstance().startQrCodeActivity()
        }

        @JvmStatic fun sendToService(actionCode: Int, body: String) {
            VPNActivity.getInstance().dispatchParcel(actionCode, body)
        }

        @JvmStatic fun saveFileAs(fileContent: String, suggestedName: String) {
            VPNActivity.getInstance().saveFile(fileContent, suggestedName)
        }

        @JvmStatic fun putTextToClipboard(text: String) {
            VPNActivity.getInstance().putToClipboard(text)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        instance = this

        val newIntent = intent
        val newIntentAction: String? = newIntent.action

        if (newIntent != null && newIntentAction != null && newIntentAction == "org.amnezia.vpn.qt.IMPORT_CONFIG") {
            configString = newIntent.getStringExtra("CONFIG")
        }
    }

    private fun startQrCodeActivity() {
        val intent = Intent(this, CameraActivity::class.java)
        startActivityForResult(intent, CAMERA_ACTION_CODE)
    }

    private fun saveFile(fileContent: String, suggestedName: String) {
        tmpFileContentToSave = fileContent

        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "text/*"
            putExtra(Intent.EXTRA_TITLE, suggestedName)
        }

        startActivityForResult(intent, CREATE_FILE_ACTION_CODE)
    }

    override fun getSystemService(name: String): Any? {
        return if (Build.VERSION.SDK_INT >= 29 && name == "clipboard") {
            // QT will always attempt to read the clipboard if content is there.
            // since we have no use of the clipboard in android 10+
            // we _can_  return null
            // And we definitely should since android 12 displays clipboard access.
            null
        } else {
            super.getSystemService(name)
        }
    }

    external fun handleBackButton(): Boolean

    external fun onServiceMessage(actionCode: Int, body: String?)
    external fun qtOnServiceConnected()
    external fun qtOnServiceDisconnected()
    external fun onActivityMessage(actionCode: Int, body: String?)

    private fun dispatchParcel(actionCode: Int, body: String) {
        if (!isBound) {
            Log.d(TAG, "dispatchParcel: not bound")
            delayedCommands.add(Pair(actionCode, body))
            return
        }

        if (delayedCommands.size > 0) {
            for (command in delayedCommands) {
                processCommand(command.first, command.second)
            }

            delayedCommands.clear()
        }

        processCommand(actionCode, body)
    }

    private fun processCommand(actionCode: Int, body: String) {
        val out: Parcel = Parcel.obtain()
        out.writeByteArray(body.toByteArray())

        try {
            vpnServiceBinder?.transact(actionCode, out, Parcel.obtain(), 0)
        } catch (e: DeadObjectException) {
            isBound = false
            vpnServiceBinder = null
            qtOnServiceDisconnected()
        } catch (e: RemoteException) {
            e.printStackTrace()
        }
    }

    override fun onNewIntent(newIntent: Intent) {
        super.onNewIntent(intent)

        setIntent(newIntent)

        val newIntentAction = newIntent.action

        if (newIntent != null && newIntentAction != null && newIntentAction == INTENT_ACTION_IMPORT_CONFIG) {
            configString = newIntent.getStringExtra("CONFIG")
        } 
    }

    override fun onResume() {
        super.onResume()

        if (configString != null && isBound) {
            sendImportConfigCommand()
        }
    }

    private fun sendImportConfigCommand() {
        if (configString != null) {
            val msg: Parcel = Parcel.obtain()
            msg.writeString(configString!!)

            try {
                vpnServiceBinder?.transact(ACTION_IMPORT_CONFIG, msg, Parcel.obtain(), 0)
            } catch (e: RemoteException) {
                e.printStackTrace()
            }

            configString = null
        }
    }

    private fun createConnection() = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, binder: IBinder) {
            vpnServiceBinder = binder

            // This is called when the connection with the service has been
            // established, giving us the object we can use to
            // interact with the service.  We are communicating with the
            // service using a Messenger, so here we get a client-side
            // representation of that from the raw IBinder object.
            if (registerBinder()){
                qtOnServiceConnected();
            } else {
                qtOnServiceDisconnected();
                return
            }

            isBound = true
        }

        override fun onServiceDisconnected(className: ComponentName) {
            vpnServiceBinder = null
            isBound = false
            qtOnServiceDisconnected();
        }
    }

    private var connection: ServiceConnection = createConnection()

    private fun registerBinder(): Boolean {
        val binder = VPNClientBinder()
        val out: Parcel = Parcel.obtain()
        out.writeStrongBinder(binder)

        try {
            // Register our IBinder Listener
            vpnServiceBinder?.transact(ACTION_REGISTER_LISTENER, out, Parcel.obtain(), 0)
            return true
        } catch (e: DeadObjectException) {
            isBound = false
            vpnServiceBinder = null
        } catch (e: RemoteException) {
            e.printStackTrace()
        }
        return false
    }

    private fun initServiceConnection() {
        // We already have a connection to the service,
        // just need to re-register the binder
        if (isBound && vpnServiceBinder!!.isBinderAlive() && registerBinder()) {
            qtOnServiceConnected()
            return
        }

        bindService(Intent(this, VPNService::class.java), connection, Context.BIND_AUTO_CREATE)
    }

    // TODO: Move all ipc codes into a shared lib.
    // this is getting out of hand.
    private val PERMISSION_TRANSACTION = 1337
    private val ACTION_REGISTER_LISTENER = 3
    private val ACTION_RESUME_ACTIVATE = 7
    private val ACTION_IMPORT_CONFIG = 11
    private val EVENT_PERMISSION_REQUIRED = 6
    private val EVENT_DISCONNECTED = 2

    private val UI_EVENT_QR_CODE_RECEIVED = 0

    fun onPermissionRequest(code: Int, data: Parcel?) {
        if (code != EVENT_PERMISSION_REQUIRED) {
            return
        }

        val x = Intent()
        x.readFromParcel(data)

        startActivityForResult(x, PERMISSION_TRANSACTION)
    }

    override protected fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode == PERMISSION_TRANSACTION) {
            // THATS US!
            if (resultCode == RESULT_OK) {
                // Prompt accepted, tell service to retry.
                dispatchParcel(ACTION_RESUME_ACTIVATE, "")
            } else {
                // Tell the Client we've disconnected
                onServiceMessage(EVENT_DISCONNECTED, "")
            }
            return
        }

        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == CAMERA_ACTION_CODE && resultCode == RESULT_OK) {
            val extra = data?.getStringExtra("result") ?: ""
            onActivityMessage(UI_EVENT_QR_CODE_RECEIVED, extra)
        }

        if (requestCode == CREATE_FILE_ACTION_CODE && resultCode == RESULT_OK) {
            data?.data?.also { uri ->
                alterDocument(uri)
            }
        }
    }

    private fun alterDocument(uri: Uri) {
        try {
            applicationContext.contentResolver.openFileDescriptor(uri, "w")?.use { fd ->
                FileOutputStream(fd.fileDescriptor).use { fos ->
                    fos.write(tmpFileContentToSave.toByteArray())
                }
            }
        } catch (e: FileNotFoundException) {
            e.printStackTrace()
        } catch (e: IOException) {
            e.printStackTrace()
        }

        tmpFileContentToSave = ""
    }

    private fun putToClipboard(text: String) {
        this.runOnUiThread {
            val clipboard = applicationContext.getSystemService(CLIPBOARD_SERVICE) as ClipboardManager?

            if (clipboard != null) {
                val clip: ClipData = ClipData.newPlainText("", text)
                clipboard.setPrimaryClip(clip)
            }
        }
    }
}
