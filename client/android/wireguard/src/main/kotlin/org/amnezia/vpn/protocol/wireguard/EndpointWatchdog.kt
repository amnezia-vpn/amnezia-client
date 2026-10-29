package org.amnezia.vpn.protocol.wireguard

/**
 * Detects a tunnel that is up on the device but no longer reaches its server,
 * so the endpoint can be resolved again from the host name in the config.
 * The rule of wireguard-tools' contrib/reresolve-dns: the latest handshake is
 * older than a rekey cycle while the device keeps sending to the peer.
 *
 * A live session with traffic or PersistentKeepalive renews its handshake every
 * 120 s (REKEY_AFTER_TIME); a failed renewal is retried for 90 s more
 * (REKEY_ATTEMPT_TIME). An idle tunnel without keepalive never handshakes and
 * sends nothing, so it is never considered stale.
 *
 * Same logic as client/core/utils/endpointWatchdog.cpp on desktop.
 * All times are milliseconds since the Unix epoch; 0 means "never".
 */
internal class EndpointWatchdog(
    private val firstRetryMs: Long,
    maxRetryMs: Long
) {
    private val maxRetryMs = maxOf(firstRetryMs, maxRetryMs)

    private var tunnelStartedMs = 0L
    private var lastHandshakeMs = 0L
    private var lastTxGrowthMs = 0L
    private var lastTxBytes = 0L
    private var haveTx = false

    private var retryMs = firstRetryMs
    private var nextRefreshMs = 0L

    /** A new tunnel was brought up: byte counters start from zero again. */
    fun tunnelStarted(nowMs: Long) {
        tunnelStartedMs = nowMs
        lastHandshakeMs = 0
        lastTxGrowthMs = 0
        lastTxBytes = 0
        haveTx = false
    }

    /** Feeds one status sample of the peer; true when the endpoint should be refreshed now. */
    fun observe(nowMs: Long, lastHandshakeMs: Long, txBytes: Long): Boolean {
        if (tunnelStartedMs == 0L) tunnelStartedMs = nowMs

        if (lastHandshakeMs > this.lastHandshakeMs) {
            this.lastHandshakeMs = lastHandshakeMs
            retryMs = firstRetryMs
            nextRefreshMs = 0
        }

        if (haveTx && txBytes > lastTxBytes) lastTxGrowthMs = nowMs
        lastTxBytes = txBytes
        haveTx = true

        val since = if (this.lastHandshakeMs > 0) this.lastHandshakeMs else tunnelStartedMs
        val noHandshake = nowMs - since > STALE_HANDSHAKE_MS
        val sending = lastTxGrowthMs > 0 && nowMs - lastTxGrowthMs <= STALE_HANDSHAKE_MS

        return noHandshake && sending && nowMs >= nextRefreshMs
    }

    /** A refresh was attempted at [nowMs]: schedules the next allowed one. */
    fun refreshAttempted(nowMs: Long) {
        nextRefreshMs = nowMs + retryMs
        retryMs = minOf(retryMs * 2, maxRetryMs)
    }

    companion object {
        const val STALE_HANDSHAKE_MS = 180_000L
    }
}
