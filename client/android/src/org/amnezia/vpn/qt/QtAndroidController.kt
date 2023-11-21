package org.amnezia.vpn.qt

/**
 * JNI functions of the AndroidController class from android_controller.cpp,
 * called by events in the Android part of the client
 */
object QtAndroidController {
    external fun onStatus(isVpnConnected: Boolean)
    external fun onServiceDisconnected()
    external fun onServiceError()

    external fun onVpnPermissionRejected()
    external fun onVpnConnected()
    external fun onVpnDisconnected()
    external fun onStatisticsUpdate(rxBytes: Long, txBytes: Long)

    external fun onConfigImported()

    external fun decodeQrCode(data: String): Boolean
}