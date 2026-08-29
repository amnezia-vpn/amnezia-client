package org.amnezia.vpn.protocol.dnstt

import org.amnezia.vpn.util.Log

private const val TAG = "libdnstt"

object DnsttNative {

    /**
     * VpnService.protect, installed by [Dnstt] for the lifetime of a tunnel.
     * libdnstt calls it through JNI before connecting to a resolver, so that
     * its own traffic is not routed back into the TUN.
     */
    @Volatile
    var protector: ((Int) -> Boolean)? = null

    /** Called from native code. Keep the name and signature in sync with jni_helper.c. */
    @JvmStatic
    fun protectSocket(fd: Int): Boolean = protector?.invoke(fd) ?: false

    /** Called from native code to surface libdnstt's log output in logcat. */
    @JvmStatic
    fun nativeLog(message: String) = Log.i(TAG, message)

    /**
     * Tunnel state sink, installed by [Dnstt] for the lifetime of a tunnel.
     * libdnstt reports "connected", "reconnecting" and "disconnected" here so
     * the UI does not keep claiming a dead tunnel is up.
     */
    @Volatile
    var stateListener: ((String) -> Unit)? = null

    /** Called from native code. Keep the name and signature in sync with jni_helper.c. */
    @JvmStatic
    fun onStateChanged(state: String) {
        Log.i(TAG, "tunnel state: $state")
        stateListener?.invoke(state)
    }

    /**
     * Starts the tunnel on [tunFd], which native code takes ownership of.
     * Returns null on success, or a human-readable failure reason.
     */
    external fun startTunnel(
        tunFd: Int,
        tunMtu: Int,
        domain: String,
        resolvers: String,
        bootstrapIp: String,
        pubKey: String
    ): String?

    /** Stops the tunnel. Returns null on success, or a failure reason. */
    external fun stopTunnel(): String?

    /** Returns the dnstt payload MTU available under [domain], or 0 if unusable. */
    external fun calculateMtu(domain: String): Int
}
