package org.amnezia.vpn

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.app.NotificationManager
import android.content.BroadcastReceiver
import android.content.ComponentName
import android.content.Intent
import android.content.Intent.EXTRA_MIME_TYPES
import android.content.Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.net.VpnService
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.Message
import android.os.Messenger
import android.provider.Settings
import android.view.MotionEvent
import android.view.WindowManager.LayoutParams
import android.webkit.MimeTypeMap
import android.widget.Toast
import androidx.annotation.MainThread
import androidx.annotation.RequiresApi
import androidx.core.content.ContextCompat
import java.io.IOException
import kotlin.LazyThreadSafetyMode.NONE
import kotlin.text.RegexOption.IGNORE_CASE
import AppListProvider
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.async
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import org.amnezia.vpn.protocol.getStatistics
import org.amnezia.vpn.protocol.getStatus
import org.amnezia.vpn.qt.QtAndroidController
import org.amnezia.vpn.util.LibraryLoader.loadSharedLibrary
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.Prefs
import org.json.JSONException
import org.json.JSONObject
import org.qtproject.qt.android.bindings.QtActivity

private const val TAG = "AmneziaActivity"
const val ACTIVITY_MESSENGER_NAME = "Activity"

private const val CHECK_VPN_PERMISSION_ACTION_CODE = 1
private const val CREATE_FILE_ACTION_CODE = 2
private const val OPEN_FILE_ACTION_CODE = 3
private const val CHECK_NOTIFICATION_PERMISSION_ACTION_CODE = 4

private const val PREFS_NOTIFICATION_PERMISSION_ASKED = "NOTIFICATION_PERMISSION_ASKED"

class AmneziaActivity : QtActivity() {

    private lateinit var mainScope: CoroutineScope
    private val qtInitialized = CompletableDeferred<Unit>()
    private var vpnProto: VpnProto? = null
    private var isWaitingStatus = true
    private var isServiceConnected = false
    private var isInBoundState = false
    private var notificationStateReceiver: BroadcastReceiver? = null
    private lateinit var vpnServiceMessenger: IpcMessenger

    private val actionResultHandlers = mutableMapOf<Int, ActivityResultHandler>()
    private val permissionRequestHandlers = mutableMapOf<Int, PermissionRequestHandler>()

    private val vpnServiceEventHandler: Handler by lazy(NONE) {
        object : Handler(Looper.getMainLooper()) {
            override fun handleMessage(msg: Message) {
                val event = msg.extractIpcMessage<ServiceEvent>()
                Log.d(TAG, "Handle event: $event")
                when (event) {
                    ServiceEvent.STATUS_CHANGED -> {
                        msg.data?.getStatus()?.let { (state) ->
                            Log.d(TAG, "Handle protocol state: $state")
                            QtAndroidController.onVpnStateChanged(state.ordinal)
                        }
                    }

                    ServiceEvent.STATUS -> {
                        if (isWaitingStatus) {
                            isWaitingStatus = false
                            msg.data?.getStatus()?.let { QtAndroidController.onStatus(it) }
                        }
                    }

                    ServiceEvent.STATISTICS_UPDATE -> {
                        msg.data?.getStatistics()?.let { (rxBytes, txBytes) ->
                            QtAndroidController.onStatisticsUpdate(rxBytes, txBytes)
                        }
                    }

                    ServiceEvent.ERROR -> {
                        msg.data?.getString(MSG_ERROR)?.let { error ->
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
                vpnServiceMessenger.send(
                    Action.REGISTER_CLIENT.packToMessage {
                        putString(MSG_CLIENT_NAME, ACTIVITY_MESSENGER_NAME)
                    },
                    replyTo = activityMessenger
                )
                isServiceConnected = true
                if (isWaitingStatus) {
                    vpnServiceMessenger.send(Action.REQUEST_STATUS, replyTo = activityMessenger)
                }
            }

            override fun onServiceDisconnected(name: ComponentName?) {
                Log.w(TAG, "Service ${name?.flattenToString()} was unexpectedly disconnected")
                isServiceConnected = false
                vpnServiceMessenger.reset()
                isWaitingStatus = true
                QtAndroidController.onServiceDisconnected()
                doBindService()
            }

            override fun onBindingDied(name: ComponentName?) {
                Log.w(TAG, "Binding to the ${name?.flattenToString()} unexpectedly died")
                doUnbindService()
                QtAndroidController.onServiceDisconnected()
                doBindService()
            }
        }
    }

    /**
     * Activity overloaded methods
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(TAG, "Create Amnezia activity")
        loadLibs()
        window.apply {
            addFlags(LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS)
            statusBarColor = getColor(R.color.black)
        }
        mainScope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
        val proto = mainScope.async(Dispatchers.IO) {
            VpnStateStore.getVpnState().vpnProto
        }
        vpnServiceMessenger = IpcMessenger(
            "VpnService",
            onDeadObjectException = {
                doUnbindService()
                QtAndroidController.onServiceDisconnected()
                doBindService()
            }
        )
        registerBroadcastReceivers()
        intent?.let(::processIntent)
        runBlocking { vpnProto = proto.await() }
    }

    private fun loadLibs() {
        listOf(
            "rsapss",
            "crypto_3",
            "ssl_3",
            "ssh"
        ).forEach {
            loadSharedLibrary(this.applicationContext, it)
        }
    }

    private fun registerBroadcastReceivers() {
        notificationStateReceiver = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            registerBroadcastReceiver(
                arrayOf(
                    NotificationManager.ACTION_NOTIFICATION_CHANNEL_BLOCK_STATE_CHANGED,
                    NotificationManager.ACTION_APP_BLOCK_STATE_CHANGED
                )
            ) {
                Log.v(
                    TAG, "Notification state changed: ${it?.action}, blocked = " +
                        "${it?.getBooleanExtra(NotificationManager.EXTRA_BLOCKED_STATE, false)}"
                )
                mainScope.launch {
                    qtInitialized.await()
                    QtAndroidController.onNotificationStateChanged()
                }
            }
        } else null
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        Log.v(TAG, "onNewIntent: $intent")
        intent?.let(::processIntent)
    }

    private fun processIntent(intent: Intent) {
        // disable config import when starting activity from history
        if (intent.flags and FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY == 0) {
            if (intent.action == ACTION_IMPORT_CONFIG) {
                intent.getStringExtra(EXTRA_CONFIG)?.let {
                    mainScope.launch {
                        qtInitialized.await()
                        QtAndroidController.onConfigImported(it)
                    }
                }
            }
        }
    }

    override fun onStart() {
        super.onStart()
        Log.d(TAG, "Start Amnezia activity")
        mainScope.launch {
            qtInitialized.await()
            vpnProto?.let { proto ->
                if (AmneziaVpnService.isRunning(applicationContext, proto.processName)) {
                    doBindService()
                }
            }
        }
    }

    override fun onStop() {
        Log.d(TAG, "Stop Amnezia activity")
        doUnbindService()
        mainScope.launch {
            qtInitialized.await()
            QtAndroidController.onServiceDisconnected()
        }
        super.onStop()
    }

    override fun onDestroy() {
        Log.d(TAG, "Destroy Amnezia activity")
        unregisterBroadcastReceiver(notificationStateReceiver)
        notificationStateReceiver = null
        mainScope.cancel()
        super.onDestroy()
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        Log.d(TAG, "Process activity result, code: ${actionCodeToString(requestCode)}, " +
                "resultCode: $resultCode, data: $data")
        actionResultHandlers[requestCode]?.let { handler ->
            when (resultCode) {
                RESULT_OK -> handler.onSuccess(data)
                else -> handler.onFail(data)
            }
            handler.onAny(data)
            actionResultHandlers.remove(requestCode)
        } ?: super.onActivityResult(requestCode, resultCode, data)
    }

    private fun startActivityForResult(intent: Intent, requestCode: Int, handler: ActivityResultHandler) {
        actionResultHandlers[requestCode] = handler
        startActivityForResult(intent, requestCode)
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        Log.d(TAG, "Process permission result, code: ${actionCodeToString(requestCode)}, " +
                "permissions: ${permissions.contentToString()}, results: ${grantResults.contentToString()}")
        permissionRequestHandlers[requestCode]?.let { handler ->
            if (grantResults.isNotEmpty()) {
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) handler.onSuccess()
                else handler.onFail()
            }
            handler.onAny()
            permissionRequestHandlers.remove(requestCode)
        } ?: super.onRequestPermissionsResult(requestCode, permissions, grantResults)
    }

    private fun requestPermission(permission: String, requestCode: Int, handler: PermissionRequestHandler) {
        permissionRequestHandlers[requestCode] = handler
        requestPermissions(arrayOf(permission), requestCode)
    }

    /**
     * Methods for service binding
     */
    @MainThread
    private fun doBindService() {
        Log.d(TAG, "Bind service")
        vpnProto?.let { proto ->
            Intent(this, proto.serviceClass).also {
                bindService(it, serviceConnection, BIND_ABOVE_CLIENT and BIND_AUTO_CREATE)
            }
            isInBoundState = true
        }
    }

    @MainThread
    private fun doUnbindService() {
        if (isInBoundState) {
            Log.d(TAG, "Unbind service")
            isWaitingStatus = true
            isServiceConnected = false
            vpnServiceMessenger.send(Action.UNREGISTER_CLIENT, activityMessenger)
            vpnServiceMessenger.reset()
            isInBoundState = false
            unbindService(serviceConnection)
        }
    }

    /**
     * Methods of starting and stopping VpnService
     */
    @MainThread
    private fun checkVpnPermission(onPermissionGranted: () -> Unit) {
        Log.d(TAG, "Check VPN permission")
        VpnService.prepare(applicationContext)?.let { intent ->
            startActivityForResult(intent, CHECK_VPN_PERMISSION_ACTION_CODE, ActivityResultHandler(
                onSuccess = {
                    Log.d(TAG, "Vpn permission granted")
                    Toast.makeText(this@AmneziaActivity, resources.getText(R.string.vpnGranted), Toast.LENGTH_LONG).show()
                    onPermissionGranted()
                },
                onFail = {
                    Log.w(TAG, "Vpn permission denied")
                    showOnVpnPermissionRejectDialog()
                    mainScope.launch {
                        qtInitialized.await()
                        QtAndroidController.onVpnPermissionRejected()
                    }
                }
            ))
        } ?: onPermissionGranted()
    }

    private fun showOnVpnPermissionRejectDialog() {
        AlertDialog.Builder(this)
            .setTitle(R.string.vpnSetupFailed)
            .setMessage(R.string.vpnSetupFailedMessage)
            .setNegativeButton(R.string.ok) { _, _ -> }
            .setPositiveButton(R.string.openVpnSettings) { _, _ ->
                startActivity(Intent(Settings.ACTION_VPN_SETTINGS))
            }
            .show()
    }

    private fun checkNotificationPermission(onChecked: () -> Unit) {
        Log.d(TAG, "Check notification permission")
        if (
            !isNotificationPermissionGranted() &&
            !Prefs.load<Boolean>(PREFS_NOTIFICATION_PERMISSION_ASKED)
        ) {
            showNotificationPermissionDialog(onChecked)
        } else {
            onChecked()
        }
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    private fun showNotificationPermissionDialog(onChecked: () -> Unit) {
        AlertDialog.Builder(this)
            .setTitle(R.string.notificationDialogTitle)
            .setMessage(R.string.notificationDialogMessage)
            .setNegativeButton(R.string.no) { _, _ ->
                Prefs.save(PREFS_NOTIFICATION_PERMISSION_ASKED, true)
                onChecked()
            }
            .setPositiveButton(R.string.yes) { _, _ ->
                val saveAsked: () -> Unit = {
                    Prefs.save(PREFS_NOTIFICATION_PERMISSION_ASKED, true)
                }
                requestPermission(
                    Manifest.permission.POST_NOTIFICATIONS,
                    CHECK_NOTIFICATION_PERMISSION_ACTION_CODE,
                    PermissionRequestHandler(
                        onSuccess = saveAsked,
                        onFail = saveAsked,
                        onAny = onChecked
                    )
                )
            }
            .show()
    }

    @MainThread
    private fun startVpn(vpnConfig: String) {
        getVpnProto(vpnConfig)?.let { proto ->
            Log.v(TAG, "Proto from config: $proto, current proto: $vpnProto")
            if (isServiceConnected) {
                if (proto.serviceClass == vpnProto?.serviceClass) {
                    vpnProto = proto
                    connectToVpn(vpnConfig)
                    return
                }
                doUnbindService()
            }
            vpnProto = proto
            isWaitingStatus = false
            startVpnService(vpnConfig, proto)
            doBindService()
        } ?: QtAndroidController.onServiceError()
    }

    private fun getVpnProto(vpnConfig: String): VpnProto? = try {
        require(vpnConfig.isNotBlank()) { "Blank VPN config" }
        VpnProto.get(JSONObject(vpnConfig).getString("protocol"))
    } catch (e: JSONException) {
        Log.e(TAG, "Invalid VPN config json format: ${e.message}")
        null
    } catch (e: IllegalArgumentException) {
        Log.e(TAG, "Protocol not found: ${e.message}")
        null
    }

    private fun connectToVpn(vpnConfig: String) {
        Log.d(TAG, "Connect to VPN")
        vpnServiceMessenger.send {
            Action.CONNECT.packToMessage {
                putString(MSG_VPN_CONFIG, vpnConfig)
            }
        }
    }

    private fun startVpnService(vpnConfig: String, proto: VpnProto) {
        Log.d(TAG, "Start VPN service: $proto")
        Intent(this, proto.serviceClass).apply {
            putExtra(MSG_VPN_CONFIG, vpnConfig)
        }.also {
            try {
                ContextCompat.startForegroundService(this, it)
            } catch (e: SecurityException) {
                Log.e(TAG, "Failed to start ${proto.serviceClass.simpleName}: $e")
                QtAndroidController.onServiceError()
            }
        }
    }

    @MainThread
    private fun disconnectFromVpn() {
        Log.d(TAG, "Disconnect from VPN")
        vpnServiceMessenger.send(Action.DISCONNECT)
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
            checkVpnPermission {
                checkNotificationPermission {
                    startVpn(vpnConfig)
                }
            }
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
    fun resetLastServer(index: Int) {
        Log.v(TAG, "Reset server: $index")
        mainScope.launch {
            VpnStateStore.store {
                if (index == -1 || it.serverIndex == index) {
                    VpnState.defaultState
                } else if (it.serverIndex > index) {
                    it.copy(serverIndex = it.serverIndex - 1)
                } else {
                    it
                }
            }
        }
    }

    @Suppress("unused")
    fun saveFile(fileName: String, data: String) {
        Log.d(TAG, "Save file $fileName")
        mainScope.launch {
            Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "text/*"
                putExtra(Intent.EXTRA_TITLE, fileName)
            }.also {
                startActivityForResult(it, CREATE_FILE_ACTION_CODE, ActivityResultHandler(
                    onSuccess = {
                        it?.data?.let { uri ->
                            Log.v(TAG, "Save file to $uri")
                            try {
                                contentResolver.openOutputStream(uri)?.use { os ->
                                    os.bufferedWriter().use { it.write(data) }
                                }
                            } catch (e: IOException) {
                                Log.e(TAG, "Failed to save file $uri: $e")
                                // todo: send error to Qt
                            }
                        }
                    }
                ))
            }
        }
    }

    @Suppress("unused")
    fun openFile(filter: String?) {
        Log.v(TAG, "Open file with filter: $filter")
        mainScope.launch {
            val mimeTypes = if (!filter.isNullOrEmpty()) {
                val extensionRegex = "\\*\\.([a-z0-9]+)".toRegex(IGNORE_CASE)
                val mime = MimeTypeMap.getSingleton()
                extensionRegex.findAll(filter).map {
                    it.groups[1]?.value?.let { mime.getMimeTypeFromExtension(it) } ?: "*/*"
                }.toSet()
            } else emptySet()

            Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                Log.v(TAG, "File mimyType filter: $mimeTypes")
                if ("*/*" in mimeTypes) {
                    type = "*/*"
                } else {
                    when (mimeTypes.size) {
                        1 -> type = mimeTypes.first()

                        in 2..Int.MAX_VALUE -> {
                            type = "*/*"
                            putExtra(EXTRA_MIME_TYPES, mimeTypes.toTypedArray())
                        }

                        else -> type = "*/*"
                    }
                }
            }.also {
                startActivityForResult(it, OPEN_FILE_ACTION_CODE, ActivityResultHandler(
                    onAny = {
                        val uri = it?.data?.toString() ?: ""
                        Log.v(TAG, "Open file: $uri")
                        mainScope.launch {
                            qtInitialized.await()
                            QtAndroidController.onFileOpened(uri)
                        }
                    }
                ))
            }
        }
    }

    @Suppress("unused")
    @SuppressLint("UnsupportedChromeOsCameraSystemFeature")
    fun isCameraPresent(): Boolean = applicationContext.packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA)

    @Suppress("unused")
    fun isOnTv(): Boolean = applicationContext.packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)

    @Suppress("unused")
    fun startQrCodeReader() {
        Log.v(TAG, "Start camera")
        Intent(this, CameraActivity::class.java).also {
            startActivity(it)
        }
    }

    @Suppress("unused")
    fun setSaveLogs(enabled: Boolean) {
        Log.v(TAG, "Set save logs: $enabled")
        mainScope.launch {
            Log.saveLogs = enabled
            vpnServiceMessenger.send {
                Action.SET_SAVE_LOGS.packToMessage {
                    putBoolean(MSG_SAVE_LOGS, enabled)
                }
            }
        }
    }

    @Suppress("unused")
    fun exportLogsFile(fileName: String) {
        Log.v(TAG, "Export logs file")
        saveFile(fileName, Log.getLogs())
    }

    @Suppress("unused")
    fun clearLogs() {
        Log.v(TAG, "Clear logs")
        mainScope.launch {
            Log.clearLogs()
        }
    }

    @Suppress("unused")
    fun setScreenshotsEnabled(enabled: Boolean) {
        Log.v(TAG, "Set screenshots enabled: $enabled")
        mainScope.launch {
            val flag = if (enabled) 0 else LayoutParams.FLAG_SECURE
            window.setFlags(flag, LayoutParams.FLAG_SECURE)
        }
    }

    @Suppress("unused")
    fun setNavigationBarColor(color: Int) {
        Log.v(TAG, "Change navigation bar color: ${"#%08X".format(color)}")
        mainScope.launch {
            window.navigationBarColor = color
        }
    }

    @Suppress("unused")
    fun minimizeApp() {
        Log.v(TAG, "Minimize application")
        mainScope.launch {
            moveTaskToBack(false)
        }
    }

    @Suppress("unused")
    fun getAppList(): String {
        Log.v(TAG, "Get app list")
        var appList = ""
        runBlocking {
            mainScope.launch {
                withContext(Dispatchers.IO) {
                    appList = AppListProvider.getAppList(packageManager, packageName)
                }
            }.join()
        }
        return appList
    }

    @Suppress("unused")
    fun getAppIcon(packageName: String, width: Int, height: Int): Bitmap {
        Log.v(TAG, "Get app icon")
        return AppListProvider.getAppIcon(packageManager, packageName, width, height)
    }

    @Suppress("unused")
    fun isNotificationPermissionGranted(): Boolean = applicationContext.isNotificationPermissionGranted()

    @Suppress("unused")
    fun requestNotificationPermission() {
        val shouldShowPreRequest = shouldShowRequestPermissionRationale(Manifest.permission.POST_NOTIFICATIONS)
        requestPermission(
            Manifest.permission.POST_NOTIFICATIONS,
            CHECK_NOTIFICATION_PERMISSION_ACTION_CODE,
            PermissionRequestHandler(
                onSuccess = {
                    mainScope.launch {
                        Prefs.save(PREFS_NOTIFICATION_PERMISSION_ASKED, true)
                        vpnServiceMessenger.send(Action.NOTIFICATION_PERMISSION_GRANTED)
                        qtInitialized.await()
                        QtAndroidController.onNotificationStateChanged()
                    }
                },
                onFail = {
                    if (!Prefs.load<Boolean>(PREFS_NOTIFICATION_PERMISSION_ASKED)) {
                        Prefs.save(PREFS_NOTIFICATION_PERMISSION_ASKED, true)
                    } else {
                        val shouldShowPostRequest =
                            shouldShowRequestPermissionRationale(Manifest.permission.POST_NOTIFICATIONS)
                        if (!shouldShowPreRequest && !shouldShowPostRequest) {
                            showNotificationSettingsDialog()
                        }
                    }
                }
            )
        )
    }

    private fun showNotificationSettingsDialog() {
        AlertDialog.Builder(this)
            .setTitle(R.string.notificationSettingsDialogTitle)
            .setMessage(R.string.notificationSettingsDialogMessage)
            .setNegativeButton(R.string.cancel) { _, _ -> }
            .setPositiveButton(R.string.openNotificationSettings) { _, _ ->
                startActivity(Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS).apply {
                    putExtra(Settings.EXTRA_APP_PACKAGE, packageName)
                })
            }
            .show()
    }

    @Suppress("unused")
    fun requestAuthentication() {
        Log.v(TAG, "Request authentication")
        mainScope.launch {
            qtInitialized.await()
            Intent(this@AmneziaActivity, AuthActivity::class.java).also {
                startActivity(it)
            }
        }
    }

    // workaround for a bug in Qt that causes the mouse click event not to be handled
    // also disable right-click, as it causes the application to crash
    private var lastButtonState = 0
    private fun MotionEvent.fixCopy(): MotionEvent = MotionEvent.obtain(
        downTime,
        eventTime,
        action,
        pointerCount,
        (0 until pointerCount).map { i ->
            MotionEvent.PointerProperties().apply {
                getPointerProperties(i, this)
            }
        }.toTypedArray(),
        (0 until pointerCount).map { i ->
            MotionEvent.PointerCoords().apply {
                getPointerCoords(i, this)
            }
        }.toTypedArray(),
        metaState,
        MotionEvent.BUTTON_PRIMARY,
        xPrecision,
        yPrecision,
        deviceId,
        edgeFlags,
        source,
        flags
    )

    private fun handleMouseEvent(ev: MotionEvent, superDispatch: (MotionEvent?) -> Boolean): Boolean {
        when (ev.action) {
            MotionEvent.ACTION_DOWN -> {
                lastButtonState = ev.buttonState
                if (ev.buttonState == MotionEvent.BUTTON_SECONDARY) return true
            }

            MotionEvent.ACTION_UP -> {
                when (lastButtonState) {
                    MotionEvent.BUTTON_SECONDARY -> return true
                    MotionEvent.BUTTON_PRIMARY -> {
                        val modEvent = ev.fixCopy()
                        return superDispatch(modEvent).apply { modEvent.recycle() }
                    }
                }
            }
        }
        return superDispatch(ev)
    }

    override fun dispatchTouchEvent(ev: MotionEvent?): Boolean {
        if (ev != null && ev.getToolType(0) == MotionEvent.TOOL_TYPE_MOUSE) {
            return handleMouseEvent(ev) { super.dispatchTouchEvent(it) }
        }
        return super.dispatchTouchEvent(ev)
    }

    override fun dispatchTrackballEvent(ev: MotionEvent?): Boolean {
        ev?.let { return handleMouseEvent(ev) { super.dispatchTrackballEvent(it) }}
        return super.dispatchTrackballEvent(ev)
    }

    /**
     * Utils methods
     */
    companion object {
        private fun actionCodeToString(actionCode: Int): String =
            when (actionCode) {
                CHECK_VPN_PERMISSION_ACTION_CODE -> "CHECK_VPN_PERMISSION"
                CREATE_FILE_ACTION_CODE -> "CREATE_FILE"
                OPEN_FILE_ACTION_CODE -> "OPEN_FILE"
                CHECK_NOTIFICATION_PERMISSION_ACTION_CODE -> "CHECK_NOTIFICATION_PERMISSION"
                else -> actionCode.toString()
            }
    }
}

private class ActivityResultHandler(
    val onSuccess: (data: Intent?) -> Unit = {},
    val onFail: (data: Intent?) -> Unit = {},
    val onAny: (data: Intent?) -> Unit = {}
)

private class PermissionRequestHandler(
    val onSuccess: () -> Unit = {},
    val onFail: () -> Unit = {},
    val onAny: () -> Unit = {}
)
