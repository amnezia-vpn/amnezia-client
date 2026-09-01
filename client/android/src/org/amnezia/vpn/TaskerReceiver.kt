package org.amnezia.vpn

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.net.VpnService
import androidx.core.content.ContextCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.util.Log

private const val TAG = "TaskerReceiver"

/**
 * Exported [BroadcastReceiver] that lets Tasker (and other automation apps) control AmneziaVPN.
 *
 * ── Tasker → AmneziaVPN (use "Send Intent" action in Tasker) ────────────────────────
 *   Package : org.amnezia.vpn
 *   Class   : org.amnezia.vpn.TaskerReceiver
 *   Target  : Broadcast Receiver
 *
 *   Action                          Constant                        Description
 *   ──────────────────────────────  ──────────────────────────────  ─────────────────────────────
 *   org.amnezia.vpn.tasker.CONNECT  [ACTION_TASKER_CONNECT]         Connect to last used server
 *   org.amnezia.vpn.tasker.DISCONNECT [ACTION_TASKER_DISCONNECT]    Disconnect from VPN
 *   org.amnezia.vpn.tasker.TOGGLE   [ACTION_TASKER_TOGGLE]          Toggle VPN on / off
 *
 * ── AmneziaVPN → Tasker (use "Intent Received" event in Tasker) ─────────────────────
 *   The app broadcasts [ACTION_TASKER_VPN_STATE_CHANGED] on every VPN state transition.
 *   Set the Tasker event action to that string; the following local variables are set:
 *
 *   Tasker variable   Extra key         Values
 *   ────────────────  ────────────────  ──────────────────────────────────────────────
 *   %state            state             CONNECTED | DISCONNECTED | CONNECTING |
 *                                       DISCONNECTING | RECONNECTING
 *   %server_name      server_name       display name of the active server (may be empty)
 *   %protocol         protocol          protocol label e.g. "AmneziaWG" (may be empty)
 */
class TaskerReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent?) {
        Log.d(TAG, "Received intent: ${intent?.action}")
        when (intent?.action) {
            ACTION_TASKER_CONNECT -> handleConnect(context)
            ACTION_TASKER_DISCONNECT -> handleDisconnect(context)
            ACTION_TASKER_TOGGLE -> handleToggle(context)
            else -> Log.w(TAG, "Unknown action: ${intent?.action}")
        }
    }

    private fun handleConnect(context: Context) {
        val pendingResult = goAsync()
        CoroutineScope(Dispatchers.IO + SupervisorJob()).launch {
            try {
                startOrConnectVpn(context)
            } catch (e: Exception) {
                Log.e(TAG, "Connect failed: $e")
            } finally {
                pendingResult.finish()
            }
        }
    }

    private fun handleDisconnect(context: Context) {
        Log.d(TAG, "Tasker: disconnect")
        context.sendBroadcast(
            Intent(ACTION_DISCONNECT).apply { setPackage(context.packageName) }
        )
    }

    private fun handleToggle(context: Context) {
        val pendingResult = goAsync()
        CoroutineScope(Dispatchers.IO + SupervisorJob()).launch {
            try {
                when (VpnStateStore.getVpnState().protocolState) {
                    ProtocolState.CONNECTED,
                    ProtocolState.CONNECTING,
                    ProtocolState.RECONNECTING -> {
                        Log.d(TAG, "Tasker: toggle → disconnect")
                        context.sendBroadcast(
                            Intent(ACTION_DISCONNECT).apply { setPackage(context.packageName) }
                        )
                    }
                    else -> {
                        Log.d(TAG, "Tasker: toggle → connect")
                        startOrConnectVpn(context)
                    }
                }
            } catch (e: Exception) {
                Log.e(TAG, "Toggle failed: $e")
            } finally {
                pendingResult.finish()
            }
        }
    }

    /**
     * Starts the VPN service for the last used protocol, or opens the app if no prior config
     * exists or VPN permission still needs to be granted.
     */
    private suspend fun startOrConnectVpn(context: Context) {
        val vpnState = VpnStateStore.getVpnState()
        val vpnProto = vpnState.vpnProto

        if (vpnState.serverName == null || vpnProto == null) {
            Log.w(TAG, "No VPN configuration found – opening main app")
            context.startActivity(
                Intent(context, AmneziaActivity::class.java).apply {
                    addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                }
            )
            return
        }

        if (VpnService.prepare(context) != null) {
            Log.d(TAG, "VPN permission not yet granted – requesting")
            context.startActivity(
                Intent(context, VpnRequestActivity::class.java).apply {
                    addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    putExtra(EXTRA_PROTOCOL, vpnProto)
                }
            )
            return
        }

        Log.d(TAG, "Tasker: connect (${vpnProto.label})")
        try {
            ContextCompat.startForegroundService(
                context,
                Intent(context, vpnProto.serviceClass)
            )
        } catch (e: SecurityException) {
            Log.e(TAG, "Failed to start ${vpnProto.serviceClass.simpleName}: $e")
        }
    }

    companion object {
        /** Connect to the last used VPN server. */
        const val ACTION_TASKER_CONNECT = "org.amnezia.vpn.tasker.CONNECT"

        /** Disconnect from VPN. */
        const val ACTION_TASKER_DISCONNECT = "org.amnezia.vpn.tasker.DISCONNECT"

        /** Toggle VPN: connect if disconnected, disconnect if connected. */
        const val ACTION_TASKER_TOGGLE = "org.amnezia.vpn.tasker.TOGGLE"

        /**
         * Broadcast fired by AmneziaVPN on every VPN state change.
         *
         * Extras:
         *   [EXTRA_STATE]        – ProtocolState name (e.g. "CONNECTED")
         *   [EXTRA_SERVER_NAME]  – server display name, empty string when unavailable
         *   [EXTRA_PROTOCOL]     – protocol label, empty string when unavailable
         */
        const val ACTION_TASKER_VPN_STATE_CHANGED = "org.amnezia.vpn.tasker.VPN_STATE_CHANGED"
        const val EXTRA_STATE = "state"
        const val EXTRA_SERVER_NAME = "server_name"
        const val EXTRA_TASKER_PROTOCOL = "protocol"
    }
}
