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
import org.amnezia.vpn.util.Log

private const val TAG = "StrictSplitTunnelGuard"

// android.os.Process.INVALID_UID
private const val INVALID_UID = -1

// UserHandle.PER_USER_RANGE; UserHandle.getAppId is not public API.
private const val PER_USER_RANGE = 100_000

// Process.FIRST_SDK_SANDBOX_UID..LAST_SDK_SANDBOX_UID: the SDK sandbox of app id N
// runs as N + 10000 (Android 13+). Process.getAppUidForSdkSandboxUid is API 35.
private const val FIRST_SDK_SANDBOX_UID = 20_000
private const val LAST_SDK_SANDBOX_UID = 29_999
private const val SDK_SANDBOX_UID_OFFSET = 10_000

/**
 * Maps a uid to the app id it belongs to, the way the platform applies a VPN's app
 * list: one entry covers every copy of the app (other users, cloned apps, a second
 * space) and its SDK sandbox.
 */
internal fun appIdOf(uid: Int): Int {
    val appId = uid % PER_USER_RANGE
    return if (appId in FIRST_SDK_SANDBOX_UID..LAST_SDK_SANDBOX_UID) appId - SDK_SANDBOX_UID_OFFSET else appId
}

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
 * feature's threat model. There is no retry: the answer is not a race to ride out,
 * and a retry would double the cost of exactly the flows an attacker sends.
 *
 * The guard keeps no state and is called from several native threads at once. It
 * is asked once per new flow; the datapath caches the verdict where it needs one.
 *
 * Apps are matched by app id ([appIdOf]), not by uid: Android applies a listed
 * package to all its copies and its SDK sandbox, so a cloned app has a uid of its
 * own and must still count as listed.
 */
class StrictSplitTunnelGuard internal constructor(
    private val mode: SplitTunnelMode,
    private val appIds: Set<Int>,
    private val ownUid: Int,
    private val resolveUid: (network: String, srcIp: String, srcPort: Int, dstIp: String, dstPort: Int) -> Int,
) {
    /**
     * Returns whether a new connection with the given 5-tuple may enter the tunnel.
     * network is "tcp"/"udp"; src is the originating app endpoint, dst the destination.
     */
    fun allow(network: String, srcIp: String, srcPort: Int, dstIp: String, dstPort: Int): Boolean {
        val uid = resolveUid(network, srcIp, srcPort, dstIp, dstPort)
        if (uid == INVALID_UID) {
            Log.w(TAG, "deny $network $srcIp:$srcPort->$dstIp:$dstPort: owner unresolved or outside this VPN")
            return false
        }
        if (uid == ownUid) return true
        val allowed = when (mode) {
            SplitTunnelMode.INCLUDE -> appIdOf(uid) in appIds   // only listed apps may tunnel
            SplitTunnelMode.EXCLUDE -> appIdOf(uid) !in appIds   // excluded apps may not
        }
        if (!allowed) {
            Log.w(TAG, "deny $network $srcIp:$srcPort->$dstIp:$dstPort: uid $uid not allowed in $mode mode")
        }
        return allowed
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
         * the split-tunnel app list for [mode]; it is resolved to app ids once here.
         */
        @RequiresApi(Build.VERSION_CODES.Q)
        fun create(context: Context, mode: SplitTunnelMode, packageNames: Set<String>): StrictSplitTunnelGuard {
            val connectivityManager = context.getSystemService<ConnectivityManager>()!!
            val packageManager = context.packageManager
            val appIds = packageNames.mapNotNullTo(HashSet()) { packageUid(packageManager, it)?.let(::appIdOf) }
            Log.i(TAG, "strict split tunneling: mode=$mode, ${appIds.size}/${packageNames.size} app ids resolved")

            return StrictSplitTunnelGuard(mode, appIds, Process.myUid()) {
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
