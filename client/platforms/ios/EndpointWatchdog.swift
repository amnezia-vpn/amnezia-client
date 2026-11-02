import Foundation

/// Detects a tunnel that is up on the device but no longer reaches its server,
/// so the endpoint can be resolved again from the host name in the config.
/// The rule of wireguard-tools' contrib/reresolve-dns: the latest handshake is
/// older than a rekey cycle while the device keeps sending to the peer.
///
/// A live session with traffic or PersistentKeepalive renews its handshake
/// every 120 s (REKEY_AFTER_TIME); a failed renewal is retried for 90 s more
/// (REKEY_ATTEMPT_TIME). An idle tunnel without keepalive never handshakes and
/// sends nothing, so it is never considered stale.
///
/// Same logic as client/core/utils/endpointWatchdog.cpp on desktop.
/// All times are milliseconds since the Unix epoch; 0 means "never".
struct EndpointWatchdog {
    static let staleHandshakeMs: Int64 = 180_000

    private let firstRetryMs: Int64
    private let maxRetryMs: Int64

    private var tunnelStartedMs: Int64 = 0
    private var lastHandshakeMs: Int64 = 0
    private var lastTxGrowthMs: Int64 = 0
    private var lastTxBytes: UInt64 = 0
    private var haveTx = false

    private var retryMs: Int64
    private var nextRefreshMs: Int64 = 0

    init(firstRetryMs: Int64, maxRetryMs: Int64) {
        self.firstRetryMs = firstRetryMs
        self.maxRetryMs = max(firstRetryMs, maxRetryMs)
        self.retryMs = firstRetryMs
    }

    /// A new tunnel was brought up: byte counters start from zero again.
    mutating func tunnelStarted(nowMs: Int64) {
        tunnelStartedMs = nowMs
        lastHandshakeMs = 0
        lastTxGrowthMs = 0
        lastTxBytes = 0
        haveTx = false
    }

    /// Feeds one status sample of the peer; true when the endpoint should be refreshed now.
    mutating func observe(nowMs: Int64, lastHandshakeMs handshakeMs: Int64, txBytes: UInt64) -> Bool {
        if tunnelStartedMs == 0 {
            tunnelStartedMs = nowMs
        }

        if handshakeMs > lastHandshakeMs {
            lastHandshakeMs = handshakeMs
            retryMs = firstRetryMs
            nextRefreshMs = 0
        }

        if haveTx && txBytes > lastTxBytes {
            lastTxGrowthMs = nowMs
        }
        lastTxBytes = txBytes
        haveTx = true

        let since = lastHandshakeMs > 0 ? lastHandshakeMs : tunnelStartedMs
        let noHandshake = nowMs - since > Self.staleHandshakeMs
        let sending = lastTxGrowthMs > 0 && nowMs - lastTxGrowthMs <= Self.staleHandshakeMs

        return noHandshake && sending && nowMs >= nextRefreshMs
    }

    /// A refresh was attempted at `nowMs`: schedules the next allowed one.
    mutating func refreshAttempted(nowMs: Int64) {
        nextRefreshMs = nowMs + retryMs
        retryMs = min(retryMs * 2, maxRetryMs)
    }
}
