package org.amnezia.vpn.util.net

import android.content.Context
import android.content.pm.PackageManager
import android.net.ConnectivityManager
import android.os.Build
import android.os.Process
import android.system.OsConstants
import androidx.annotation.RequiresApi
import androidx.core.content.getSystemService
import java.net.InetSocketAddress
import java.util.concurrent.ConcurrentHashMap
import org.amnezia.vpn.util.Log

private const val TAG = "StrictSplitTunnelGuard"

// android.os.Process.INVALID_UID
private const val INVALID_UID = -1

// Positive-resolution cache TTL. UID lookups happen once per new connection, so
// the cache mostly absorbs retransmits / rapid re-dials; keep it short.
private const val CACHE_TTL_MS = 10_000L
private const val CACHE_MAX_ENTRIES = 2048

enum class SplitTunnelMode { INCLUDE, EXCLUDE }

/**
 * Per-connection gate backing the "Strict Split Tunneling" feature (issue #2457):
 * it stops apps that bypass the OS split-tunnel rules (via SO_BINDTODEVICE on
 * tun0) from leaking traffic into the tunnel.
 *
 * The decision logic is pure Kotlin with an injected [resolveUid], so it can be
 * exercised without Android; the production resolver (ConnectivityManager-backed)
 * is built by [create].
 *
 * An unresolved owner (INVALID_UID) is denied (fail-closed). Android returns it
 * both when no socket matches and when the owner is not covered by this VPN, so
 * an app outside the split-tunnel rules lands here, which is the intended deny.
 * Crafting an ownerless packet needs raw sockets (root), which is outside this
 * feature's threat model.
 */
class StrictSplitTunnelGuard internal constructor(
    private val mode: SplitTunnelMode,
    private val appUids: Set<Int>,
    private val ownUid: Int,
    private val resolveUid: (network: String, srcIp: String, srcPort: Int, dstIp: String, dstPort: Int) -> Int,
) {
    private class CachedUid(val uid: Int, val expiresAt: Long)

    private val cache = ConcurrentHashMap<String, CachedUid>()

    /**
     * Returns whether a new connection with the given 5-tuple may enter the tunnel.
     * network is "tcp"/"udp"; src is the originating app endpoint, dst the destination.
     */
    fun allow(network: String, srcIp: String, srcPort: Int, dstIp: String, dstPort: Int): Boolean {
        val uid = ownerUid(network, srcIp, srcPort, dstIp, dstPort)
        if (uid == INVALID_UID) {
            Log.w(TAG, "deny $network $srcIp:$srcPort->$dstIp:$dstPort: owner unresolved or outside this VPN")
            return false
        }
        if (uid == ownUid) return true
        return when (mode) {
            SplitTunnelMode.INCLUDE -> uid in appUids   // only listed apps may tunnel
            SplitTunnelMode.EXCLUDE -> uid !in appUids   // excluded apps may not
        }
    }

    private fun ownerUid(network: String, srcIp: String, srcPort: Int, dstIp: String, dstPort: Int): Int {
        val key = "$network/$srcIp/$srcPort"
        val now = System.currentTimeMillis()
        cache[key]?.let { if (it.expiresAt > now) return it.uid }

        var uid = resolveUid(network, srcIp, srcPort, dstIp, dstPort)
        if (uid == INVALID_UID) {
            // One immediate re-query to ride out a socket-table lookup race.
            uid = resolveUid(network, srcIp, srcPort, dstIp, dstPort)
        }
        if (uid != INVALID_UID) {
            if (cache.size > CACHE_MAX_ENTRIES) {
                cache.entries.removeIf { it.value.expiresAt <= now }
            }
            cache[key] = CachedUid(uid, now + CACHE_TTL_MS)
        }
        return uid
    }

    companion object {
        /**
         * Builds a guard for the app split-tunnel lists of a protocol config, or returns
         * null when there is nothing to enforce: app split tunneling is off, or the
         * platform is older than API 29, where the owner cannot be resolved.
         */
        fun createOrNull(
            context: Context,
            includedApps: Set<String>,
            excludedApps: Set<String>,
        ): StrictSplitTunnelGuard? {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) return null
            return when {
                includedApps.isNotEmpty() -> create(context, SplitTunnelMode.INCLUDE, includedApps)
                excludedApps.isNotEmpty() -> create(context, SplitTunnelMode.EXCLUDE, excludedApps)
                else -> null
            }
        }

        /**
         * Builds a guard whose resolver uses
         * [ConnectivityManager.getConnectionOwnerUid] (API 29+). [packageNames] is
         * the split-tunnel app list for [mode]; it is resolved to UIDs once here.
         */
        @RequiresApi(Build.VERSION_CODES.Q)
        fun create(context: Context, mode: SplitTunnelMode, packageNames: Set<String>): StrictSplitTunnelGuard {
            val connectivityManager = context.getSystemService<ConnectivityManager>()!!
            val packageManager = context.packageManager
            val appUids = packageNames.mapNotNullTo(HashSet()) { packageUid(packageManager, it) }
            Log.i(TAG, "strict split tunneling: mode=$mode, ${appUids.size}/${packageNames.size} app uids resolved")

            return StrictSplitTunnelGuard(mode, appUids, Process.myUid()) {
                    network, srcIp, srcPort, dstIp, dstPort ->
                val protocol = when (network) {
                    "tcp" -> OsConstants.IPPROTO_TCP
                    "udp" -> OsConstants.IPPROTO_UDP
                    else -> -1
                }
                if (protocol < 0) {
                    INVALID_UID
                } else {
                    try {
                        connectivityManager.getConnectionOwnerUid(
                            protocol,
                            InetSocketAddress(parseInetAddress(srcIp), srcPort),
                            InetSocketAddress(parseInetAddress(dstIp), dstPort),
                        )
                    } catch (e: Exception) {
                        Log.e(TAG, "getConnectionOwnerUid failed: $e")
                        INVALID_UID
                    }
                }
            }
        }

        private fun packageUid(packageManager: PackageManager, packageName: String): Int? =
            try {
                packageManager.getPackageUid(packageName, 0)
            } catch (e: PackageManager.NameNotFoundException) {
                Log.w(TAG, "package not found while resolving uid: $packageName")
                null
            }
    }
}
