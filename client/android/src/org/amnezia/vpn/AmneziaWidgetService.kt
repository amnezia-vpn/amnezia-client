package org.amnezia.vpn

import android.app.Service
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import android.os.Messenger
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import org.amnezia.vpn.util.Log

private const val TAG = "AmneziaWidgetService"
private const val ACTION_SEND_DISCONNECT = "org.amnezia.vpn.WIDGET_SEND_DISCONNECT"
private const val EXTRA_VPN_PROTO = "VPN_PROTO"

/**
 * Background service that:
 * 1. Listens to VpnStateStore changes and pushes widget UI updates
 * 2. Handles VPN disconnect requests from the widget (needs Service lifecycle
 *    to reliably bind to the VPN service — BroadcastReceiver is too short-lived)
 */
class AmneziaWidgetService : Service() {

    private lateinit var scope: CoroutineScope
    private var stateListeningJob: Job? = null

    // Disconnect binding state
    private var disconnectConnection: ServiceConnection? = null

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "Widget service created")
        scope = CoroutineScope(SupervisorJob())
        startStateListening()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        // Re-ensure state listening is active (in case of restart)
        if (stateListeningJob == null || stateListeningJob?.isActive != true) {
            startStateListening()
        }

        when (intent?.action) {
            ACTION_SEND_DISCONNECT -> {
                val proto = if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
                    intent.getSerializableExtra(EXTRA_VPN_PROTO, VpnProto::class.java)
                } else {
                    @Suppress("DEPRECATION")
                    intent.getSerializableExtra(EXTRA_VPN_PROTO) as? VpnProto
                }
                if (proto != null) {
                    handleDisconnect(proto)
                }
            }
        }
        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        Log.d(TAG, "Widget service destroyed")
        stateListeningJob?.cancel()
        cleanupDisconnectBinding()
        scope.cancel()
        super.onDestroy()
    }

    private fun startStateListening() {
        stateListeningJob = scope.launch {
            VpnStateStore.dataFlow().collectLatest { vpnState ->
                Log.d(TAG, "VPN state changed: $vpnState")
                AmneziaWidgetProvider.updateAllWidgets(applicationContext, vpnState)
            }
        }
    }

    /**
     * Bind to VPN service and send DISCONNECT in the onServiceConnected callback.
     * This is reliable because:
     * - We're a Service (long lifecycle), not a BroadcastReceiver
     * - We send the message in onServiceConnected (guaranteed the connection is ready)
     * - We unbind immediately after sending
     */
    private fun handleDisconnect(vpnProto: VpnProto) {
        // Clean up any previous disconnect binding
        cleanupDisconnectBinding()

        val connection = object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
                Log.d(TAG, "Bound to VPN service for disconnect: ${name?.flattenToString()}")
                try {
                    val messenger = IpcMessenger(
                        Messenger(service),
                        "WidgetDisconnect"
                    )
                    messenger.send(Action.DISCONNECT)
                    Log.d(TAG, "Disconnect message sent")
                } catch (e: Exception) {
                    Log.e(TAG, "Failed to send disconnect: $e")
                }
                // Unbind after sending
                cleanupDisconnectBinding()
            }

            override fun onServiceDisconnected(name: ComponentName?) {
                Log.w(TAG, "VPN service disconnected unexpectedly: ${name?.flattenToString()}")
            }
        }

        disconnectConnection = connection

        try {
            val bound = bindService(
                Intent(this, vpnProto.serviceClass),
                connection,
                BIND_AUTO_CREATE
            )
            if (!bound) {
                Log.e(TAG, "Failed to bind to VPN service for disconnect")
                disconnectConnection = null
            }
        } catch (e: Exception) {
            Log.e(TAG, "Exception binding for disconnect: $e")
            disconnectConnection = null
        }
    }

    private fun cleanupDisconnectBinding() {
        disconnectConnection?.let { conn ->
            try {
                unbindService(conn)
            } catch (e: Exception) {
                Log.e(TAG, "Failed to unbind disconnect connection: $e")
            }
            disconnectConnection = null
        }
    }

    companion object {
        fun start(context: Context) {
            try {
                context.startService(Intent(context, AmneziaWidgetService::class.java))
            } catch (e: Exception) {
                Log.e(TAG, "Failed to start widget service: $e")
            }
        }

        fun stop(context: Context) {
            context.stopService(Intent(context, AmneziaWidgetService::class.java))
        }

        fun sendDisconnect(context: Context, vpnProto: VpnProto) {
            Intent(context, AmneziaWidgetService::class.java).apply {
                action = ACTION_SEND_DISCONNECT
                putExtra(EXTRA_VPN_PROTO, vpnProto)
            }.also {
                context.startService(it)
            }
        }
    }
}
