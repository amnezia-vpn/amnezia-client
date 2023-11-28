package org.amnezia.vpn

import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.net.Uri
import android.net.VpnService
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.Message
import android.os.Messenger
import android.widget.Toast
import androidx.annotation.MainThread
import androidx.core.content.ContextCompat
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.IOException
import kotlin.LazyThreadSafetyMode.NONE
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import org.amnezia.vpn.protocol.getStatistics
import org.amnezia.vpn.protocol.getStatus
import org.amnezia.vpn.qt.QtAndroidController
import org.amnezia.vpn.util.Log
import org.qtproject.qt.android.bindings.QtActivity

private const val TAG = "AmneziaActivity"

private const val CHECK_VPN_PERMISSION_ACTION_CODE = 1
private const val CREATE_FILE_ACTION_CODE = 2

class AmneziaActivity : QtActivity() {

    private lateinit var mainScope: CoroutineScope
    private val qtInitialized = CompletableDeferred<Unit>()
    private var isWaitingStatus = true
    private var isServiceConnected = false
    private var isInBoundState = false
    private lateinit var vpnServiceMessenger: IpcMessenger
    private var tmpFileContentToSave: String = ""

    private val vpnServiceEventHandler: Handler by lazy(NONE) {
        object : Handler(Looper.getMainLooper()) {
            override fun handleMessage(msg: Message) {
                val event = msg.extractIpcMessage<ServiceEvent>()
                Log.d(TAG, "Handle event: $event")
                when (event) {
                    ServiceEvent.CONNECTED -> {
                        QtAndroidController.onVpnConnected()
                    }

                    ServiceEvent.DISCONNECTED -> {
                        QtAndroidController.onVpnDisconnected()
                    }

                    ServiceEvent.STATUS -> {
                        if (isWaitingStatus) {
                            isWaitingStatus = false
                            msg.data?.getStatus()?.let { (isConnected) ->
                                QtAndroidController.onStatus(isConnected)
                            }
                        }
                    }

                    ServiceEvent.STATISTICS_UPDATE -> {
                        msg.data?.getStatistics()?.let { (rxBytes, txBytes) ->
                            QtAndroidController.onStatisticsUpdate(rxBytes, txBytes)
                        }
                    }

                    ServiceEvent.ERROR -> {
                        msg.data?.getString(ERROR_MSG)?.let { error ->
                            Log.e(TAG, "From VpnService: $error")
                        }
                        // todo: add error reporting to Qt
                        QtAndroidController.onServiceError()
                    }
                }
            }
        }
    }

    private val activityMessenger: Messenger by lazy(NONE) {
        Messenger(vpnServiceEventHandler)
    }

    private val serviceConnection: ServiceConnection by lazy(NONE) {
        object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
                Log.d(TAG, "Service ${name?.flattenToString()} was connected")
                // get a messenger from the service to send actions to the service
                vpnServiceMessenger.set(Messenger(service))
                // send a messenger to the service to process service events
                vpnServiceMessenger.send {
                    Action.REGISTER_CLIENT.packToMessage().apply {
                        replyTo = activityMessenger
                    }
                }
                isServiceConnected = true
                if (isWaitingStatus) {
                    vpnServiceMessenger.send(Action.REQUEST_STATUS)
                }
            }

            override fun onServiceDisconnected(name: ComponentName?) {
                Log.w(TAG, "Service ${name?.flattenToString()} was unexpectedly disconnected")
                isServiceConnected = false
                vpnServiceMessenger.reset()
                isWaitingStatus = true
                QtAndroidController.onServiceDisconnected()
            }

            override fun onBindingDied(name: ComponentName?) {
                Log.w(TAG, "Binding to the ${name?.flattenToString()} unexpectedly died")
                doUnbindService()
                doBindService()
            }
        }
    }

    private data class CheckVpnPermissionCallbacks(val onSuccess: () -> Unit, val onFail: () -> Unit)

    private var checkVpnPermissionCallbacks: CheckVpnPermissionCallbacks? = null

    /**
     * Activity overloaded methods
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.v(TAG, "Create Amnezia activity")
        mainScope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
        vpnServiceMessenger = IpcMessenger(
            onDeadObjectException = ::doUnbindService,
            messengerName = "VpnService"
        )
    }

    override fun onStart() {
        super.onStart()
        Log.v(TAG, "Start Amnezia activity")
        mainScope.launch {
            qtInitialized.await()
            doBindService()
        }
    }

    override fun onStop() {
        Log.v(TAG, "Stop Amnezia activity")
        doUnbindService()
        super.onStop()
    }

    override fun onDestroy() {
        Log.v(TAG, "Destroy Amnezia activity")
        mainScope.cancel()
        super.onDestroy()
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            CREATE_FILE_ACTION_CODE -> {
                when (resultCode) {
                    RESULT_OK -> {
                        data?.data?.let { uri ->
                            alterDocument(uri)
                        }
                    }
                }
            }

            CHECK_VPN_PERMISSION_ACTION_CODE -> {
                when (resultCode) {
                    RESULT_OK -> {
                        Log.v(TAG, "Vpn permission granted")
                        Toast.makeText(this, "Vpn permission granted", Toast.LENGTH_LONG).show()
                        checkVpnPermissionCallbacks?.run { onSuccess() }
                    }

                    else -> {
                        Log.w(TAG, "Vpn permission denied, resultCode: $resultCode")
                        Toast.makeText(this, "Vpn permission denied", Toast.LENGTH_LONG).show()
                        checkVpnPermissionCallbacks?.run { onFail() }
                    }
                }
                checkVpnPermissionCallbacks = null
            }

            else -> super.onActivityResult(requestCode, resultCode, data)
        }
    }

    /**
     * Methods for service binding
     */
    @MainThread
    private fun doBindService() {
        Log.v(TAG, "Bind service")
        Intent(this, AmneziaVpnService::class.java).also {
            bindService(it, serviceConnection, BIND_ABOVE_CLIENT)
        }
        isInBoundState = true
    }

    @MainThread
    private fun doUnbindService() {
        if (isInBoundState) {
            Log.v(TAG, "Unbind service")
            isWaitingStatus = true
            QtAndroidController.onServiceDisconnected()
            vpnServiceMessenger.reset()
            isServiceConnected = false
            isInBoundState = false
            unbindService(serviceConnection)
        }
    }

    /**
     * Methods of starting and stopping VpnService
     */
    private fun checkVpnPermissionAndStart(vpnConfig: String) {
        checkVpnPermission(
            onSuccess = { startVpn(vpnConfig) },
            onFail = QtAndroidController::onVpnPermissionRejected
        )
    }

    @MainThread
    private fun checkVpnPermission(onSuccess: () -> Unit, onFail: () -> Unit) {
        Log.v(TAG, "Check VPN permission")
        VpnService.prepare(applicationContext)?.let {
            checkVpnPermissionCallbacks = CheckVpnPermissionCallbacks(onSuccess, onFail)
            startActivityForResult(it, CHECK_VPN_PERMISSION_ACTION_CODE)
            return
        }
        onSuccess()
    }

    @MainThread
    private fun startVpn(vpnConfig: String) {
        if (isServiceConnected) {
            connectToVpn(vpnConfig)
        } else {
            isWaitingStatus = false
            startVpnService(vpnConfig)
            doBindService()
        }
    }

    private fun connectToVpn(vpnConfig: String) {
        Log.v(TAG, "Connect to VPN")
        vpnServiceMessenger.send {
            Action.CONNECT.packToMessage {
                putString(VPN_CONFIG, vpnConfig)
            }
        }
    }

    private fun startVpnService(vpnConfig: String) {
        Log.v(TAG, "Start VPN service")
        Intent(this, AmneziaVpnService::class.java).apply {
            putExtra(VPN_CONFIG, vpnConfig)
        }.also {
            ContextCompat.startForegroundService(this, it)
        }
    }

    private fun disconnectFromVpn() {
        Log.v(TAG, "Disconnect from VPN")
        vpnServiceMessenger.send(Action.DISCONNECT)
    }

    // saving file
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

    /**
     * Methods called by Qt
     */
    @Suppress("unused")
    fun qtAndroidControllerInitialized() {
        Log.v(TAG, "Qt Android controller initialized")
        qtInitialized.complete(Unit)
    }

    @Suppress("unused")
    fun start(vpnConfig: String) {
        Log.v(TAG, "Start VPN")
        mainScope.launch {
            checkVpnPermissionAndStart(vpnConfig)
        }
    }

    @Suppress("unused")
    fun stop() {
        Log.v(TAG, "Stop VPN")
        mainScope.launch {
            disconnectFromVpn()
        }
    }

    @Suppress("unused")
    fun saveFile(fileName: String, data: String) {
        Log.v(TAG, "Save file $fileName")
        // todo: refactor
        tmpFileContentToSave = data

        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "text/*"
            putExtra(Intent.EXTRA_TITLE, fileName)
        }

        startActivityForResult(intent, CREATE_FILE_ACTION_CODE)
    }

    @Suppress("unused")
    fun setNotificationText(title: String, message: String, timerSec: Int) {
        Log.v(TAG, "Set notification text")
        Log.w(TAG, "Not yet implemented")
    }

    @Suppress("unused")
    fun cleanupLogs() {
        Log.v(TAG, "Cleanup logs")
        Log.w(TAG, "Not yet implemented")
    }

    @Suppress("unused")
    fun startQrCodeReader() {
        Log.v(TAG, "Start camera")
        Intent(this, CameraActivity::class.java).also {
            startActivity(it)
        }
    }
}
