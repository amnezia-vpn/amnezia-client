package com.fblink.vpn.protocol

// keep synchronized with client/platforms/android/android_controller.h ConnectionState
enum class ProtocolState {
    DISCONNECTED,
    CONNECTED,
    CONNECTING,
    DISCONNECTING,
    RECONNECTING,
    UNKNOWN
}
