package org.amnezia.vpn

import android.app.Notification
import android.app.PendingIntent
import android.content.Intent
import android.content.pm.ServiceInfo.FOREGROUND_SERVICE_TYPE_MANIFEST
import android.content.pm.ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE
import android.net.VpnService
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.Message
import android.os.Messenger
import androidx.annotation.MainThread
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat
import kotlin.LazyThreadSafetyMode.NONE
import kotlinx.coroutines.CoroutineExceptionHandler
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeoutOrNull
import org.amnezia.vpn.protocol.BadConfigException
import org.amnezia.vpn.protocol.LoadLibraryException
import org.amnezia.vpn.protocol.Protocol
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.CONNECTING
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTING
import org.amnezia.vpn.protocol.ProtocolState.UNKNOWN
import org.amnezia.vpn.protocol.Statistics
import org.amnezia.vpn.protocol.Status
import org.amnezia.vpn.protocol.VpnException
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.protocol.awg.Awg
import org.amnezia.vpn.protocol.cloak.Cloak
import org.amnezia.vpn.protocol.openvpn.OpenVpn
import org.amnezia.vpn.protocol.putStatistics
import org.amnezia.vpn.protocol.putStatus
import org.amnezia.vpn.protocol.wireguard.Wireguard
import org.amnezia.vpn.util.Log
import org.json.JSONException
import org.json.JSONObject

private const val TAG = "AmneziaVpnService"

const val VPN_CONFIG = "VPN_CONFIG"
const val ERROR_MSG = "ERROR_MSG"
const val AFTER_PERMISSION_CHECK = "AFTER_PERMISSION_CHECK"
private const val PREFS_CONFIG_KEY = "LAST_CONF"
private const val NOTIFICATION_ID = 1337
private const val STATISTICS_SENDING_TIMEOUT = 1000L
private const val DISCONNECT_TIMEOUT = 1500L

class AmneziaVpnService : VpnService() {

    private lateinit var mainScope: CoroutineScope
    private lateinit var connectionScope: CoroutineScope
    private var isServiceBound = false
    private var protocol: Protocol? = null
    private val protocolCache = mutableMapOf<String, Protocol>()
    private var protocolState = MutableStateFlow(UNKNOWN)

    private val isConnected
        get() = protocolState.value == CONNECTED

    private val isDisconnected
        get() = protocolState.value == DISCONNECTED

    private val isUnknown
        get() = protocolState.value == UNKNOWN

    private var connectionJob: Job? = null
    private var disconnectionJob: Job? = null
    private var statisticsSendingJob: Job? = null
    private lateinit var clientMessenger: IpcMessenger

    private val connectionExceptionHandler = CoroutineExceptionHandler { _, e ->
        protocolState.value = DISCONNECTED
        protocol = null
        when (e) {
            is IllegalArgumentException,
            is VpnStartException,
            is VpnException -> onError(e.message ?: e.toString())

            is JSONException,
            is BadConfigException -> onError("VPN config format error: ${e.message}")

            is LoadLibraryException -> onError("${e.message}. Caused: ${e.cause?.message}")

            else -> throw e
        }
    }

    private val actionMessageHandler: Handler by lazy(NONE) {
        object : Handler(Looper.getMainLooper()) {
            override fun handleMessage(msg: Message) {
                val action = msg.extractIpcMessage<Action>()
                Log.d(TAG, "Handle action: $action")
                when (action) {
                    Action.REGISTER_CLIENT -> {
                        clientMessenger.set(msg.replyTo)
                    }

                    Action.CONNECT -> {
                        val vpnConfig = msg.data.getString(VPN_CONFIG)
                        saveConfigToPrefs(vpnConfig)
                        connect(vpnConfig)
                    }

                    Action.DISCONNECT -> {
                        disconnect()
                    }

                    Action.REQUEST_STATUS -> {
                        clientMessenger.send {
                            ServiceEvent.STATUS.packToMessage {
                                putStatus(Status.build {
                                    setState(this@AmneziaVpnService.protocolState.value)
                                })
                            }
                        }
                    }
                }
            }
        }
    }

    private val vpnServiceMessenger: Messenger by lazy(NONE) {
        Messenger(actionMessageHandler)
    }

    /**
     * Notification setup
     */
    private val foregroundServiceTypeCompat
        get() = when {
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE -> FOREGROUND_SERVICE_TYPE_SPECIAL_USE
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q -> FOREGROUND_SERVICE_TYPE_MANIFEST
            else -> 0
        }

    private val notification: Notification by lazy(NONE) {
        NotificationCompat.Builder(this, NOTIFICATION_CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_amnezia_round)
            .setShowWhen(false)
            .setContentIntent(
                PendingIntent.getActivity(
                    this,
                    0,
                    Intent(this, AmneziaActivity::class.java),
                    PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
                )
            )
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
            .setForegroundServiceBehavior(NotificationCompat.FOREGROUND_SERVICE_IMMEDIATE)
            .build()
    }

    /**
     * Service overloaded methods
     */
    override fun onCreate() {
        super.onCreate()
        Log.v(TAG, "Create Amnezia VPN service")
        mainScope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
        connectionScope = CoroutineScope(SupervisorJob() + Dispatchers.IO + connectionExceptionHandler)
        clientMessenger = IpcMessenger(messengerName = "Client")
        launchProtocolStateHandler()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val isAlwaysOnCompat =
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) isAlwaysOn
            else intent?.component?.packageName != packageName

        if (isAlwaysOnCompat) {
            Log.v(TAG, "Start service via Always-on")
            connect(loadConfigFromPrefs())
        } else if (intent?.getBooleanExtra(AFTER_PERMISSION_CHECK, false) == true) {
            Log.v(TAG, "Start service after permission check")
            connect(loadConfigFromPrefs())
        } else {
            Log.v(TAG, "Start service")
            val vpnConfig = intent?.getStringExtra(VPN_CONFIG)
            saveConfigToPrefs(vpnConfig)
            connect(vpnConfig)
        }
        ServiceCompat.startForeground(this, NOTIFICATION_ID, notification, foregroundServiceTypeCompat)
        return START_REDELIVER_INTENT
    }

    override fun onBind(intent: Intent?): IBinder? {
        Log.d(TAG, "onBind by $intent")
        if (intent?.action == "android.net.VpnService") return super.onBind(intent)
        isServiceBound = true
        if (isConnected) launchSendingStatistics()
        return vpnServiceMessenger.binder
    }

    override fun onUnbind(intent: Intent?): Boolean {
        Log.d(TAG, "onUnbind by $intent")
        if (intent?.action != "android.net.VpnService") {
            isServiceBound = false
            stopSendingStatistics()
            clientMessenger.reset()
            if (isUnknown || isDisconnected) stopSelf()
        }
        return true
    }

    override fun onRebind(intent: Intent?) {
        Log.d(TAG, "onRebind by $intent")
        if (intent?.action != "android.net.VpnService") {
            isServiceBound = true
            if (isConnected) launchSendingStatistics()
        }
        super.onRebind(intent)
    }

    override fun onRevoke() {
        Log.v(TAG, "onRevoke")
        // Calls to onRevoke() method may not happen on the main thread of the process
        mainScope.launch {
            disconnect()
        }
    }

    override fun onDestroy() {
        Log.v(TAG, "Destroy service")
        runBlocking {
            withTimeoutOrNull(DISCONNECT_TIMEOUT) {
                disconnect()
                disconnectionJob?.join()
            }
        }
        connectionScope.cancel()
        mainScope.cancel()
        super.onDestroy()
    }

    /**
     * Methods responsible for processing VPN connection
     */
    private fun launchProtocolStateHandler() {
        mainScope.launch {
            protocolState.collect { protocolState ->
                Log.d(TAG, "Protocol state: $protocolState")
                when (protocolState) {
                    CONNECTED -> {
                        clientMessenger.send(ServiceEvent.CONNECTED)
                        if (isServiceBound) launchSendingStatistics()
                    }

                    DISCONNECTED -> {
                        clientMessenger.send(ServiceEvent.DISCONNECTED)
                        stopSendingStatistics()
                        if (!isServiceBound) stopSelf()
                    }

                    DISCONNECTING -> {
                        stopSendingStatistics()
                    }

                    CONNECTING, UNKNOWN -> {}
                }
            }
        }
    }

    @MainThread
    private fun launchSendingStatistics() {
        if (isServiceBound && isConnected) {
            statisticsSendingJob = mainScope.launch {
                while (true) {
                    clientMessenger.send {
                        ServiceEvent.STATISTICS_UPDATE.packToMessage {
                            putStatistics(protocol?.statistics ?: Statistics.EMPTY_STATISTICS)
                        }
                    }
                    delay(STATISTICS_SENDING_TIMEOUT)
                }
            }
        }
    }

    @MainThread
    private fun stopSendingStatistics() {
        statisticsSendingJob?.cancel()
    }

    @MainThread
    private fun connect(vpnConfig: String?) {
        Log.v(TAG, "Start VPN connection")

        if (isConnected || protocolState.value == CONNECTING) return

        protocolState.value = CONNECTING

        val config = parseConfigToJson(vpnConfig)
        if (config == null) {
            onError("Invalid VPN config")
            protocolState.value = DISCONNECTED
            return
        }

        if (!checkPermission()) {
            protocolState.value = DISCONNECTED
            return
        }

        connectionJob = connectionScope.launch {
            disconnectionJob?.join()
            disconnectionJob = null

            protocol = getProtocol(config.getString("protocol"))
            protocol?.startVpn(config, Builder(), ::protect)
        }
    }

    @MainThread
    private fun disconnect() {
        Log.v(TAG, "Stop VPN connection")

        if (isUnknown || isDisconnected || protocolState.value == DISCONNECTING) return

        protocolState.value = DISCONNECTING

        disconnectionJob = connectionScope.launch {
            connectionJob?.join()
            connectionJob = null

            protocol?.stopVpn()
            protocol = null
        }
    }

    @MainThread
    private fun getProtocol(protocolName: String): Protocol =
        protocolCache[protocolName]
            ?: when (protocolName) {
                "wireguard" -> Wireguard()
                "awg" -> Awg()
                "openvpn" -> OpenVpn()
                "cloak" -> Cloak()
                else -> throw IllegalArgumentException("Protocol '$protocolName' not found")
            }.apply { initialize(applicationContext, protocolState, ::onError) }
                .also { protocolCache[protocolName] = it }

    /**
     * Utils methods
     */
    private fun onError(msg: String) {
        Log.e(TAG, msg)
        mainScope.launch {
            clientMessenger.send {
                ServiceEvent.ERROR.packToMessage {
                    putString(ERROR_MSG, msg)
                }
            }
        }
    }

    private fun parseConfigToJson(vpnConfig: String?): JSONObject? =
        try {
            vpnConfig?.let {
                JSONObject(it)
            }
        } catch (e: JSONException) {
            onError("Invalid VPN config json format: ${e.message}")
            null
        }

    private fun checkPermission(): Boolean =
        if (prepare(applicationContext) != null) {
            Intent(this, VpnRequestActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }.also {
                startActivity(it)
            }
            false
        } else {
            true
        }

    private fun loadConfigFromPrefs(): String? =
        Prefs.get(this).getString(PREFS_CONFIG_KEY, null)

    private fun saveConfigToPrefs(config: String?) =
        Prefs.get(this).edit().putString(PREFS_CONFIG_KEY, config).apply()
}
