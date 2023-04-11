/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Parcel
import androidx.core.app.NotificationCompat
import org.json.JSONObject

object NotificationUtil {
    var sCurrentContext: Context? = null
    private var sNotificationBuilder: NotificationCompat.Builder? = null

    const val NOTIFICATION_CHANNEL_ID = "com.amnezia.vpnNotification"
    const val CONNECTED_NOTIFICATION_ID = 1337
    const val tag = "NotificationUtil"

    /**
     * Updates the current shown notification from a
     * Parcel - Gets called from AndroidController.cpp
     */
    fun update(data: Parcel) {
        // [data] is here a json containing the notification content
        val buffer = data.createByteArray()
        val json = buffer?.let { String(it) }
        val content = JSONObject(json)

        update(content.getString("title"), content.getString("message"))
    }

    /**
     * Updates the current shown notification
     */
    fun update(heading: String, message: String) {
        if (sCurrentContext == null) return
        val notificationManager: NotificationManager =
            sCurrentContext?.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

        sNotificationBuilder?.let {
            it.setContentTitle(heading)
                .setContentText(message)
            notificationManager.notify(CONNECTED_NOTIFICATION_ID, it.build())
        }
    }

    /**
     * Saves the default translated "connected" notification, in case the vpn gets started
     * without the app.
     */
    fun saveFallBackMessage(data: Parcel, context: Context) {
        // [data] is here a json containing the notification content
        val buffer = data.createByteArray()
        val json = buffer?.let { String(it) }
        val content = JSONObject(json)

        val prefs = Prefs.get(context)
        prefs.edit()
            .putString("fallbackNotificationHeader", content.getString("title"))
            .putString("fallbackNotificationMessage", content.getString("message"))
            .apply()
        Log.v(tag, "Saved new fallback message -> ${content.getString("title")}")
    }

    /*
    * Creates a new Notification using the current set of Strings
    * Shows the notification in the given {context}
    */
    fun show(service: VPNService) {
        sNotificationBuilder = NotificationCompat.Builder(service, NOTIFICATION_CHANNEL_ID)
        sCurrentContext = service
        val notificationManager: NotificationManager =
            sCurrentContext?.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        // From Oreo on we need to have a "notification channel" to post to.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val name = "vpn"
            val descriptionText = "  "
            val importance = NotificationManager.IMPORTANCE_LOW
            val channel = NotificationChannel(NOTIFICATION_CHANNEL_ID, name, importance).apply {
                description = descriptionText
            }
            // Register the channel with the system
            notificationManager.createNotificationChannel(channel)
        }
        // In case we do not have gotten a message to show from the Frontend
        // try to populate the notification with a translated Fallback message
        val prefs = Prefs.get(service)
        val message =
            "" + prefs.getString("fallbackNotificationMessage", "Running in the Background")
        val header = "" + prefs.getString("fallbackNotificationHeader", "Mozilla VPN")

        // Create the Intent that Should be Fired if the User Clicks the notification
        val mainActivityName = "org.amnezia.vpn.qt.VPNActivity"
        val activity = Class.forName(mainActivityName)
        val intent = Intent(service, activity)
        val pendingIntent = PendingIntent.getActivity(service, 0, intent, PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT)
        // Build our notification
        sNotificationBuilder?.let {
            it.setSmallIcon(org.amnezia.vpn.R.drawable.ic_amnezia_round)
                .setContentTitle(header)
                .setContentText(message)
                .setOnlyAlertOnce(true)
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setContentIntent(pendingIntent)

            service.startForeground(CONNECTED_NOTIFICATION_ID, it.build())
        }
    }
}
