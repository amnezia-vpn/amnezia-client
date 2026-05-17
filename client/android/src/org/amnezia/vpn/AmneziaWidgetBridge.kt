package org.amnezia.vpn

import android.content.Context
import android.content.Intent

class AmneziaWidgetBridge {
    companion object {
        init {
            // Имя библиотеки зависит от конфигурации сборки Amnezia (CMake/QMake).
            // Обычно это имя целевого исполняемого файла, например "amneziavpn" или "amnezia-client".
            System.loadLibrary("amneziavpn") 
        }

        // Этот метод мы будем вызывать из AmneziaWidgetProvider
        @JvmStatic
        external fun toggleVpn()
        
        // Этот метод будет вызывать C++ код при изменении статуса VPN
        @JvmStatic
        fun updateWidgetState(context: Context, isConnected: Boolean) {
            val intent = Intent(context, AmneziaWidgetProvider::class.java).apply {
                action = AmneziaWidgetProvider.ACTION_UPDATE_WIDGET_STATE
                putExtra(AmneziaWidgetProvider.EXTRA_IS_CONNECTED, isConnected)
            }
            // Рассылаем Intent, который поймает наш AmneziaWidgetProvider и обновит UI
            context.sendBroadcast(intent)
        }
    }
}