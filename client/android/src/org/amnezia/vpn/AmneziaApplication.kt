package org.amnezia.vpn

import androidx.camera.camera2.Camera2Config
import androidx.camera.core.CameraSelector
import androidx.camera.core.CameraXConfig
import androidx.core.app.NotificationChannelCompat.Builder
import androidx.core.app.NotificationManagerCompat
import org.qtproject.qt.android.bindings.QtApplication

const val NOTIFICATION_CHANNEL_ID: String = "org.amnezia.vpn.notification"

class AmneziaApplication : QtApplication(), CameraXConfig.Provider {

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun getCameraXConfig(): CameraXConfig = CameraXConfig.Builder
        .fromConfig(Camera2Config.defaultConfig())
        .setMinimumLoggingLevel(android.util.Log.ERROR)
        .setAvailableCamerasLimiter(CameraSelector.DEFAULT_BACK_CAMERA)
        .build()

    private fun createNotificationChannel() {
        NotificationManagerCompat.from(this).createNotificationChannel(
            Builder(NOTIFICATION_CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_LOW)
                .setName("AmneziaVPN")
                .setDescription("AmneziaVPN service notification")
                .setShowBadge(false)
                .build()
        )
    }
}
