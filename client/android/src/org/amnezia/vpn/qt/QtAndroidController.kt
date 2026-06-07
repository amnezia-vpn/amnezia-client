package org.amnezia.vpn.qt

import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.Status

/**
 * JNI functions of the AndroidController class from android_controller.cpp,
 * called by events in the Android part of the client
 */
object QtAndroidController {

    fun onStatus(status: Status, serverIndex: Int) = onStatus(status.state, serverIndex)
    fun onStatus(protocolState: ProtocolState, serverIndex: Int) = onStatus(protocolState.ordinal, serverIndex)

    external fun onStatus(stateCode: Int, serverIndex: Int)
    external fun onServiceDisconnected()
    external fun onServiceError()

    external fun onVpnPermissionRejected()
    external fun onNotificationStateChanged()
    external fun onVpnStateChanged(stateCode: Int)
    external fun onStatisticsUpdate(rxBytes: Long, txBytes: Long)

    external fun onFileOpened(uri: String)

    /** Notifies C++ that Android opened the system APK installer for a downloaded update. */
    external fun onApkInstallerStarted(fileName: String)

    external fun onConfigImported(data: String)

    external fun onAuthResult(result: Boolean)

    external fun decodeQrCode(data: String): Boolean

    external fun onImeInsetsChanged(heightDp: Int)
    external fun onSystemBarsInsetsChanged(navBarHeightDp: Int, statusBarHeightDp: Int)

    external fun onActivityPaused()
    external fun onActivityResumed()
}
