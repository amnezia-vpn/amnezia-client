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
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.CONNECTING
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.ProtocolState.RECONNECTING
import org.amnezia.vpn.util.Log

private const val TAG = "AmneziaWidgetService"
private const val ACTION_SEND_DISCONNECT = "org.amnezia.vpn.WIDGET_SEND_DISCONNECT"
private const val EXTRA_VPN_PROTO = "VPN_PROTO"

class AmneziaWidgetService : Service() {

    private lateinit var scope: CoroutineScope
    private var stateListeningJob: Job? = null
    private var vpnServiceMessenger: IpcMessenger? = null
    private var isInBoundState = false

    private val serviceConnection: ServiceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            Log.d(TAG, "VPN service connected: ${name?.flattenToString()}")
            vpnServiceMessenger?.set(Messenger(service))
        }

        override fun onServiceDisconnected(name: ComponentName?) {
            Log.w(TAG, "VPN service disconnected: ${name?.flattenToString()}")
            vpnServiceMessenger?.reset()
        }

        override fun onBindingDied(name: ComponentName?) {
            Log.w(TAG, "Binding died: ${name?.flattenToString()}")
            doUnbindService()
        }
    }

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "Widget service created")
        scope = CoroutineScope(SupervisorJob())
        vpnServiceMessenger = IpcMessenger(
            "WidgetVpnService",
            onDeadObjectException = ::doUnbindService
        )
        startStateListening()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
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
        doUnbindService()
        scope.cancel()
        super.onDestroy()
    }

    private fun startStateListening() {
        stateListeningJob = scope.launch {
            VpnStateStore.dataFlow().collectLatest { vpnState ->
                Log.d(TAG, "VPN state changed: $vpnState")
                AmneziaWidgetProvider.updateAllWidgets(applicationContext, vpnState)

                // Bind/unbind to VPN service based on state
                val proto = vpnState.vpnProto
                when (vpnState.protocolState) {
                    CONNECTED, CONNECTING, RECONNECTING -> {
                        if (proto != null && !isInBoundState) {
                            doBindService(proto)
                        }
                    }
                    else -> {
                        if (isInBoundState) {
                            doUnbindService()
                        }
                    }
                }
            }
        }
    }

    private fun handleDisconnect(vpnProto: VpnProto) {
        if (!isInBoundState) {
            doBindService(vpnProto)
            // Post disconnect after bind
            scope.launch {
                // Small delay to let bind complete
                kotlinx.coroutines.delay(500)
                vpnServiceMessenger?.send(Action.DISCONNECT)
            }
        } else {
            vpnServiceMessenger?.send(Action.DISCONNECT)
        }
    }

    private fun doBindService(vpnProto: VpnProto) {
        Log.d(TAG, "Binding to VPN service: ${vpnProto.serviceClass.simpleName}")
        try {
            bindService(
                Intent(this, vpnProto.serviceClass),
                serviceConnection,
                BIND_ABOVE_CLIENT
            )
            isInBoundState = true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to bind to VPN service: $e")
        }
    }

    private fun doUnbindService() {
        if (isInBoundState) {
            Log.d(TAG, "Unbinding from VPN service")
            try {
                unbindService(serviceConnection)
            } catch (e: Exception) {
                Log.e(TAG, "Failed to unbind: $e")
            }
            isInBoundState = false
            vpnServiceMessenger?.reset()
        }
    }

    companion object {
        fun start(context: Context) {
            Intent(context, AmneziaWidgetService::class.java).also {
                context.startService(it)
            }
        }

        fun stop(context: Context) {
            Intent(context, AmneziaWidgetService::class.java).also {
                context.stopService(it)
            }
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
