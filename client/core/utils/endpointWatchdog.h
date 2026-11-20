#ifndef ENDPOINTWATCHDOG_H
#define ENDPOINTWATCHDOG_H

#include <cstdint>

// Detects a WireGuard/AWG tunnel that is up on the device but no longer
// reaches its server, so the endpoint can be resolved again from the host
// name in the config. This is the rule of wireguard-tools'
// contrib/reresolve-dns: the latest handshake is older than a rekey cycle
// while the device keeps sending to the peer.
//
// A live session with traffic or PersistentKeepalive renews its handshake
// every 120 s (REKEY_AFTER_TIME); a failed renewal is retried for 90 s more
// (REKEY_ATTEMPT_TIME). An idle tunnel without keepalive never handshakes
// and sends nothing, so it is never considered stale.
//
// All times are milliseconds since the Unix epoch; 0 means "never".
class EndpointWatchdog
{
public:
    static constexpr int64_t kStaleHandshakeMs = 180 * 1000;

    // firstRetryMs / maxRetryMs: how soon a refresh may be repeated while the
    // tunnel stays stale; the delay doubles after every attempt and resets on
    // the next successful handshake.
    explicit EndpointWatchdog(int64_t firstRetryMs = 3 * 60 * 1000, int64_t maxRetryMs = 30 * 60 * 1000);

    // A new tunnel was brought up: byte counters start from zero again.
    // The retry delay is kept, so a reconnect that did not help is not
    // repeated right away.
    void tunnelStarted(int64_t nowMs);

    // Feed one status sample of the peer. Returns true when the endpoint
    // should be refreshed now.
    bool observe(int64_t nowMs, int64_t lastHandshakeMs, uint64_t txBytes);

    // A refresh was attempted at nowMs: schedule the next allowed one.
    void refreshAttempted(int64_t nowMs);

private:
    void resetRetry();

    const int64_t m_firstRetryMs;
    const int64_t m_maxRetryMs;

    int64_t m_tunnelStartedMs = 0;
    int64_t m_lastHandshakeMs = 0;
    int64_t m_lastTxGrowthMs = 0;
    uint64_t m_lastTxBytes = 0;
    bool m_haveTx = false;

    int64_t m_retryMs;
    int64_t m_nextRefreshMs = 0;
};

#endif // ENDPOINTWATCHDOG_H
