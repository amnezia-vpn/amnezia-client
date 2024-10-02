package org.amnezia.vpn

import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.net.VpnService
import android.os.IBinder
import android.os.Messenger
import android.service.quicksettings.Tile
import android.service.quicksettings.TileService
import androidx.core.content.ContextCompat
import kotlin.LazyThreadSafetyMode.NONE
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.CONNECTING
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTING
import org.amnezia.vpn.protocol.ProtocolState.RECONNECTING
import org.amnezia.vpn.protocol.ProtocolState.UNKNOWN
import org.amnezia.vpn.util.Log

private const val TAG = "AmneziaTileService"
private const val DEFAULT_TILE_LABEL = "AmneziaVPN"

class AmneziaTileService : TileService() {

    private lateinit var scope: CoroutineScope
    private var vpnStateListeningJob: Job? = null
    private lateinit var vpnServiceMessenger: IpcMessenger

    @Volatile
    private var isServiceConnected = false

    @Volatile
    private var vpnProto: VpnProto? = null
    private var isInBoundState = false
    @Volatile
    private var isVpnConfigExists = false

    private val serviceConnection: ServiceConnection by lazy(NONE) {
        object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
                Log.d(TAG, "Service ${name?.flattenToString()} was connected")
                vpnServiceMessenger.set(Messenger(service))
                isServiceConnected = true
            }

            override fun onServiceDisconnected(name: ComponentName?) {
                Log.w(TAG, "Service ${name?.flattenToString()} was unexpectedly disconnected")
                isServiceConnected = false
                vpnServiceMessenger.reset()
                updateVpnState(DISCONNECTED)
            }

            override fun onBindingDied(name: ComponentName?) {
                Log.w(TAG, "Binding to the ${name?.flattenToString()} unexpectedly died")
                doUnbindService()
                doBindService()
            }
        }
    }

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "Create Amnezia Tile Service")
        scope = CoroutineScope(SupervisorJob())
        vpnServiceMessenger = IpcMessenger(
            "VpnService",
            onDeadObjectException = ::doUnbindService
        )
    }

    override fun onDestroy() {
        Log.d(TAG, "Destroy Amnezia Tile Service")
        doUnbindService()
        scope.cancel()
        super.onDestroy()
    }

    // Workaround for some bugs
    override fun onBind(intent: Intent?): IBinder? =
        try {
            super.onBind(intent)
        } catch (e: Throwable) {
            Log.e(TAG, "Failed to bind AmneziaTileService: $e")
            null
        }

    override fun onStartListening() {
        super.onStartListening()
        scope.launch {
            Log.d(TAG, "Start listening")
            vpnProto = VpnStateStore.getVpnState().vpnProto
            vpnProto.also { proto ->
                if (proto != null && AmneziaVpnService.isRunning(applicationContext, proto.processName)) {
                    Log.d(TAG, "Vpn service is running")
                    doBindService()
                } else {
                    Log.d(TAG, "Vpn service is not running")
                    isServiceConnected = false
                    updateVpnState(DISCONNECTED)
                }
            }
            vpnStateListeningJob = launchVpnStateListening()
        }
    }

    override fun onStopListening() {
        Log.d(TAG, "Stop listening")
        vpnStateListeningJob?.cancel()
        vpnStateListeningJob = null
        doUnbindService()
        super.onStopListening()
    }

    override fun onClick() {
        Log.d(TAG, "onClick")
        if (isLocked) {
            unlockAndRun { onClickInternal() }
        } else {
            onClickInternal()
        }
    }

    private fun onClickInternal() {
        if (isVpnConfigExists && vpnProto != null) {
            Log.d(TAG, "Change VPN state")
            if (qsTile.state == Tile.STATE_INACTIVE) {
                Log.d(TAG, "Start VPN")
                updateVpnState(CONNECTING)
                startVpn()
            } else if (qsTile.state == Tile.STATE_ACTIVE) {
                Log.d(TAG, "Stop vpn")
                updateVpnState(DISCONNECTING)
                stopVpn()
            }
        } else {
            Log.d(TAG, "Start Activity")
            Intent(this, AmneziaActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }.also {
                @Suppress("DEPRECATION")
                startActivityAndCollapse(it)
            }
        }
    }

    private fun doBindService() {
        Log.d(TAG, "Bind service")
        vpnProto?.let { proto ->
            Intent(this, proto.serviceClass).also {
                bindService(it, serviceConnection, BIND_ABOVE_CLIENT)
            }
            isInBoundState = true
        }
    }

    private fun doUnbindService() {
        if (isInBoundState) {
            Log.d(TAG, "Unbind service")
            isServiceConnected = false
            vpnServiceMessenger.reset()
            isInBoundState = false
            unbindService(serviceConnection)
        }
    }

    private fun startVpn() {
        if (isServiceConnected) {
            connectToVpn()
        } else {
            if (checkPermission()) {
                startVpnService()
                doBindService()
            } else {
                updateVpnState(DISCONNECTED)
            }
        }
    }

    private fun checkPermission() =
        if (VpnService.prepare(applicationContext) != null) {
            Intent(this, VpnRequestActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                putExtra(EXTRA_PROTOCOL, vpnProto)
            }.also {
                @Suppress("DEPRECATION")
                startActivityAndCollapse(it)
            }
            false
        } else {
            true
        }

    private fun startVpnService() {
        vpnProto?.let { proto ->
            try {
                ContextCompat.startForegroundService(
                    applicationContext,
                    Intent(this, proto.serviceClass)
                )
            } catch (e: SecurityException) {
                Log.e(TAG, "Failed to start ${proto.serviceClass.simpleName}: $e")
            }
        } ?: Log.e(TAG, "Failed to start vpn service: vpnProto is null")
    }

    private fun connectToVpn() = vpnServiceMessenger.send(Action.CONNECT)

    private fun stopVpn() = vpnServiceMessenger.send(Action.DISCONNECT)

    private fun updateVpnState(state: ProtocolState) =
        scope.launch { VpnStateStore.store { it.copy(protocolState = state) } }

    private fun launchVpnStateListening() =
        scope.launch { VpnStateStore.dataFlow().collectLatest(::updateTile) }

    private fun updateTile(vpnState: VpnState) {
        Log.d(TAG, "Update tile: $vpnState")
        isVpnConfigExists = vpnState.serverName != null
        vpnProto = vpnState.vpnProto
        val tile = qsTile ?: return
        tile.apply {
            label = (vpnState.serverName ?: DEFAULT_TILE_LABEL) + (vpnProto?.let { " ${it.label}" } ?: "")
            when (val protocolState = vpnState.protocolState) {
                CONNECTED -> {
                    state = Tile.STATE_ACTIVE
                }

                DISCONNECTED, UNKNOWN -> {
                    state = Tile.STATE_INACTIVE
                }

                CONNECTING, DISCONNECTING, RECONNECTING -> {
                    state = Tile.STATE_UNAVAILABLE
                }
            }
            updateTile()
        }
        // double update to fix weird visual glitches
        tile.updateTile()
    }
}
