package org.amnezia.vpn.protocol.masterdnsvpn

/**
 * JNI bridge to the native MasterDnsVPN engine.
 *
 * The native implementation lives in client/masterdnsvpn/android_jni.cpp and
 * is compiled into the main Qt-for-Android shared library that the activity
 * already loads — no separate System.loadLibrary call needed here.
 *
 * All methods are static (object members) and serialise on a process-global
 * mutex on the C++ side; safe to call from any Kotlin coroutine context.
 */
internal object MasterDnsVpnNative {

    /** Engine::State enum, mirrored from C++. */
    const val STATE_IDLE = 0
    const val STATE_STARTING = 1
    const val STATE_CONNECTED = 2
    const val STATE_STOPPING = 3
    const val STATE_FAILED = 4

    /**
     * Spin up the native engine with the structured JSON config the Kotlin
     * Protocol class composes from the connect-time wrapper. Returns true
     * on a successful synchronous start (the SOCKS5 listener will bind
     * shortly after); false when the config fails validation.
     */
    @JvmStatic
    external fun nativeStart(configJson: String): Boolean

    /** Tear down the engine. Idempotent; safe to call when already stopped. */
    @JvmStatic
    external fun nativeStop()

    /**
     * Local SOCKS5 listen port (0 if not yet bound). Caller polls this after
     * nativeStart() returns true; tun2socks dials it once non-zero.
     */
    @JvmStatic
    external fun nativeSocksPort(): Int

    /** Engine state ordinal — see STATE_* constants above. */
    @JvmStatic
    external fun nativeState(): Int

    /** Human-readable last error from the engine (empty string when none). */
    @JvmStatic
    external fun nativeLastError(): String

    @JvmStatic
    external fun nativeBytesReceived(): Long

    @JvmStatic
    external fun nativeBytesSent(): Long
}
