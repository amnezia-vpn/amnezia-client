package org.amnezia.vpn

import android.app.PendingIntent
import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.widget.RemoteViews

class AmneziaWidgetProvider : AppWidgetProvider() {

    companion object {
        const val ACTION_TOGGLE_VPN = "org.amnezia.vpn.ACTION_TOGGLE_VPN"
        const val ACTION_UPDATE_WIDGET_STATE = "org.amnezia.vpn.ACTION_UPDATE_WIDGET_STATE"
        const val EXTRA_IS_CONNECTED = "is_connected"
    }

    override fun onUpdate(context: Context, appWidgetManager: AppWidgetManager, appWidgetIds: IntArray) {
        for (appWidgetId in appWidgetIds) {
            updateWidgetUI(context, appWidgetManager, appWidgetId, false) // По умолчанию красный/выключен
        }
    }

    override fun onReceive(context: Context, intent: Intent) {
        super.onReceive(context, intent)

        if (intent.action == ACTION_TOGGLE_VPN) {
            // Отправляем команду в C++ ядро через наш JNI-мост
            AmneziaWidgetBridge.toggleVpn()
        } 
        else if (intent.action == ACTION_UPDATE_WIDGET_STATE) {
            // Получаем ответ от C++ сервиса о новом статусе
            val isConnected = intent.getBooleanExtra(EXTRA_IS_CONNECTED, false)
            val appWidgetManager = AppWidgetManager.getInstance(context)
            
            // Получаем ID всех активных виджетов Amnezia на рабочих столах
            val componentName = ComponentName(context, AmneziaWidgetProvider::class.java)
            val appWidgetIds = appWidgetManager.getAppWidgetIds(componentName)
            
            // Обновляем UI для каждого виджета
            for (appWidgetId in appWidgetIds) {
                updateWidgetUI(context, appWidgetManager, appWidgetId, isConnected)
            }
        }
    }

    private fun updateWidgetUI(context: Context, appWidgetManager: AppWidgetManager, appWidgetId: Int, isConnected: Boolean) {
        val views = RemoteViews(context.packageName, R.layout.widget_amnezia)

        // Меняем цвет (приглушенный зеленый / красный)
        val colorRes = if (isConnected) R.color.amnezia_widget_green else R.color.amnezia_widget_red
        val colorInt = context.resources.getColor(colorRes, context.theme)
        
        // Устанавливаем цвет фона (tint)
        views.setInt(R.id.widget_button_toggle, "setColorFilter", colorInt)

        // Создаем Интент для клика по виджету
        val intent = Intent(context, AmneziaWidgetProvider::class.java).apply {
            action = ACTION_TOGGLE_VPN
        }
        val pendingIntent = PendingIntent.getBroadcast(
            context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        views.setOnClickPendingIntent(R.id.widget_button_toggle, pendingIntent)

        appWidgetManager.updateAppWidget(appWidgetId, views)
    }
}