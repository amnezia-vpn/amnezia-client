package org.amnezia.vpn.protocol

// keep synchronized with client/platforms/android/android_controller.h ConnectionState
enum class ProtocolState {
    CONNECTED,
    CONNECTING,
    DISCONNECTED,
    DISCONNECTING,
    UNKNOWN
}
