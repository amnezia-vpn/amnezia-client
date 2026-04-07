package org.amnezia.vpn

import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.IBinder
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import org.amnezia.vpn.util.Log

private const val TAG = "AmneziaWidgetService"

/**
 * Lightweight background service that listens to VpnStateStore changes
 * and pushes updates to all widget instances via RemoteViews.
 *
 * Started when the first widget is added, restarted on every onUpdate()
 * to recover from OS kills, stopped when the last widget is removed.
 */
class AmneziaWidgetService : Service() {

    private lateinit var scope: CoroutineScope
    private var stateListeningJob: Job? = null

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
        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        Log.d(TAG, "Widget service destroyed")
        stateListeningJob?.cancel()
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
    }
}
