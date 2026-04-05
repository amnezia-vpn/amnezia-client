package org.amnezia.vpn

import android.app.PendingIntent
import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.net.VpnService
import android.os.Build
import android.view.View
import android.widget.RemoteViews
import androidx.core.content.ContextCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.CONNECTING
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTING
import org.amnezia.vpn.protocol.ProtocolState.RECONNECTING
import org.amnezia.vpn.protocol.ProtocolState.UNKNOWN
import org.amnezia.vpn.util.Log

private const val TAG = "AmneziaWidgetProvider"
const val ACTION_WIDGET_TOGGLE = "org.amnezia.vpn.WIDGET_TOGGLE"

class AmneziaWidgetProvider : AppWidgetProvider() {

    override fun onUpdate(
        context: Context,
        appWidgetManager: AppWidgetManager,
        appWidgetIds: IntArray
    ) {
        Log.d(TAG, "onUpdate: ${appWidgetIds.contentToString()}")
        for (appWidgetId in appWidgetIds) {
            updateWidget(context, appWidgetManager, appWidgetId)
        }
    }

    override fun onReceive(context: Context, intent: Intent) {
        super.onReceive(context, intent)
        Log.d(TAG, "onReceive: ${intent.action}")
        when (intent.action) {
            ACTION_WIDGET_TOGGLE -> handleToggle(context)
        }
    }

    override fun onEnabled(context: Context) {
        super.onEnabled(context)
        Log.d(TAG, "Widget enabled, starting state listener service")
        AmneziaWidgetService.start(context)
    }

    override fun onDisabled(context: Context) {
        super.onDisabled(context)
        Log.d(TAG, "Widget disabled, stopping state listener service")
        AmneziaWidgetService.stop(context)
    }

    private fun handleToggle(context: Context) {
        val scope = CoroutineScope(SupervisorJob())
        scope.launch {
            try {
                val vpnState = VpnStateStore.getVpnState()
                val vpnProto = vpnState.vpnProto
                val isVpnConfigExists = vpnState.serverName != null

                if (!isVpnConfigExists || vpnProto == null) {
                    Log.d(TAG, "No VPN config, launching main activity")
                    Intent(context, AmneziaActivity::class.java).apply {
                        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    }.also {
                        context.startActivity(it)
                    }
                    return@launch
                }

                when (vpnState.protocolState) {
                    DISCONNECTED, UNKNOWN -> {
                        Log.d(TAG, "Starting VPN")
                        VpnStateStore.store { it.copy(protocolState = CONNECTING) }
                        startVpn(context, vpnProto)
                    }

                    CONNECTED -> {
                        Log.d(TAG, "Stopping VPN")
                        VpnStateStore.store { it.copy(protocolState = DISCONNECTING) }
                        AmneziaWidgetService.sendDisconnect(context, vpnProto)
                    }

                    CONNECTING, DISCONNECTING, RECONNECTING -> {
                        Log.d(TAG, "VPN is in transitional state: ${vpnState.protocolState}, ignoring toggle")
                    }
                }
            } finally {
                scope.cancel()
            }
        }
    }

    private fun startVpn(context: Context, vpnProto: VpnProto) {
        if (VpnService.prepare(context) != null) {
            Log.d(TAG, "VPN permission not granted, launching permission request")
            Intent(context, VpnRequestActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                putExtra(EXTRA_PROTOCOL, vpnProto)
            }.also {
                context.startActivity(it)
            }
            return
        }

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
        fun updateAllWidgets(context: Context, vpnState: VpnState) {
            val appWidgetManager = AppWidgetManager.getInstance(context)
            val widgetIds = appWidgetManager.getAppWidgetIds(
                ComponentName(context, AmneziaWidgetProvider::class.java)
            )
            for (widgetId in widgetIds) {
                updateWidgetWithState(context, appWidgetManager, widgetId, vpnState)
            }
        }

        private fun updateWidget(
            context: Context,
            appWidgetManager: AppWidgetManager,
            appWidgetId: Int
        ) {
            val scope = CoroutineScope(SupervisorJob())
            scope.launch {
                try {
                    val vpnState = VpnStateStore.getVpnState()
                    updateWidgetWithState(context, appWidgetManager, appWidgetId, vpnState)
                } finally {
                    scope.cancel()
                }
            }
        }

        private fun updateWidgetWithState(
            context: Context,
            appWidgetManager: AppWidgetManager,
            appWidgetId: Int,
            vpnState: VpnState
        ) {
            val views = RemoteViews(context.packageName, R.layout.widget_vpn_toggle)

            // Set circle drawable based on state
            val (circleDrawable, pulseVisible, iconTint) = when (vpnState.protocolState) {
                CONNECTED -> Triple(
                    R.drawable.widget_circle_connected,
                    View.GONE,
                    0xFFFFFFFF.toInt()
                )
                DISCONNECTED, UNKNOWN -> Triple(
                    R.drawable.widget_circle_disconnected,
                    View.GONE,
                    0xFF616161.toInt()
                )
                CONNECTING, RECONNECTING -> Triple(
                    R.drawable.widget_circle_connecting,
                    View.VISIBLE,
                    0xFFFFFFFF.toInt()
                )
                DISCONNECTING -> Triple(
                    R.drawable.widget_circle_disconnecting,
                    View.VISIBLE,
                    0xFFFFFFFF.toInt()
                )
            }

            views.setImageViewResource(R.id.widget_circle, circleDrawable)
            views.setViewVisibility(R.id.widget_pulse_ring, pulseVisible)
            views.setInt(R.id.widget_icon, "setColorFilter", iconTint)

            // Set click action
            val toggleIntent = Intent(context, AmneziaWidgetProvider::class.java).apply {
                action = ACTION_WIDGET_TOGGLE
            }
            val pendingIntent = PendingIntent.getBroadcast(
                context,
                0,
                toggleIntent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
            views.setOnClickPendingIntent(R.id.widget_root, pendingIntent)

            appWidgetManager.updateAppWidget(appWidgetId, views)
        }
    }
}
