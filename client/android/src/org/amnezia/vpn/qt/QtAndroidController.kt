package org.amnezia.vpn.qt

import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.Status

/**
 * JNI functions of the AndroidController class from android_controller.cpp,
 * called by events in the Android part of the client
 */
object QtAndroidController {

    fun onStatus(status: Status) = onStatus(status.state)
    fun onStatus(protocolState: ProtocolState) = onStatus(protocolState.ordinal)

    external fun onStatus(stateCode: Int)
    external fun onServiceDisconnected()
    external fun onServiceError()

    external fun onVpnPermissionRejected()
    external fun onNotificationStateChanged()
    external fun onVpnStateChanged(stateCode: Int)
    external fun onStatisticsUpdate(rxBytes: Long, txBytes: Long)

    external fun onFileOpened(uri: String)

    external fun onConfigImported(data: String)

    external fun onAuthResult(result: Boolean)

    external fun decodeQrCode(data: String): Boolean
}