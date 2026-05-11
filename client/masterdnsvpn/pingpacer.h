// SPDX-License-Identifier: GPL-3.0-or-later
//
// Spec §12 tiered ping pacing — extracted from Session so the tier-selection
// math (`nextIntervalMs`) and packet-notification bookkeeping
// (`notifyPacket`) are unit-testable without a live event loop.
//
// Layout follows upstream's `PingManager` (internal/client/ping_manager.go):
//
//   * Four timestamps track the most recent ping/non-ping send and the most
//     recent pong/non-pong receive.
//   * `nextIntervalMs(now)` consults them against three thresholds and
//     returns the interval that should currently gate the next PING.
//   * Aggressive / lazy / cooldown / cold tiers correspond to upstream's
//     four interval knobs, defaulted to the values shipped in
//     `internal/config/client.go:175-181`.

#ifndef MASTERDNSVPN_PINGPACER_H
#define MASTERDNSVPN_PINGPACER_H

#include "wireframing.h"

#include <QtGlobal>
#include <algorithm>

namespace amnezia::masterdnsvpn {

struct PingPacingConfig {
    qint64 aggressiveMs   = 100;     // PING_AGGRESSIVE_INTERVAL_SECONDS
    qint64 lazyMs         = 750;     // PING_LAZY_INTERVAL_SECONDS
    qint64 cooldownMs     = 2000;    // PING_COOLDOWN_INTERVAL_SECONDS
    qint64 coldMs         = 15000;   // PING_COLD_INTERVAL_SECONDS
    qint64 warmThreshMs   = 8000;    // PING_WARM_THRESHOLD_SECONDS
    qint64 coolThreshMs   = 20000;   // PING_COOL_THRESHOLD_SECONDS
    qint64 coldThreshMs   = 30000;   // PING_COLD_THRESHOLD_SECONDS
};

struct PingPacingState {
    qint64 lastPingSentMs    = 0;
    qint64 lastPongRecvMs    = 0;
    qint64 lastNonPingSentMs = 0;
    qint64 lastNonPongRecvMs = 0;

    // Seed all four timestamps to `now`. Used at session start so the FSM
    // begins in the aggressive tier (matches upstream's `newPingManager`).
    void seed(qint64 now)
    {
        lastPingSentMs    = now;
        lastPongRecvMs    = now;
        lastNonPingSentMs = now;
        lastNonPongRecvMs = now;
    }

    // Record an inbound or outbound packet. PING/PONG move ping-specific
    // timestamps; everything else moves the conversation timestamps that
    // drive tier selection.
    void notify(PacketType type, bool inbound, qint64 now)
    {
        if (inbound) {
            if (type == PacketType::Pong) {
                lastPongRecvMs = now;
            } else {
                lastNonPongRecvMs = now;
            }
        } else {
            if (type == PacketType::Ping) {
                lastPingSentMs = now;
            } else {
                lastNonPingSentMs = now;
            }
        }
    }
};

// Compute the interval that should currently gate the next PING. Mirrors
// `PingManager.nextInterval` (internal/client/ping_manager.go:106-134) — if
// either direction has carried non-ping/non-pong traffic inside the warm
// threshold, we're an active conversation and stay aggressive. Otherwise
// the minimum idle duration across the two directions promotes us through
// lazy → cooldown → cold.
inline qint64 pingNextIntervalMs(const PingPacingConfig &cfg,
                                 const PingPacingState &state,
                                 qint64 now)
{
    const qint64 idleSent = now - state.lastNonPingSentMs;
    const qint64 idleRecv = now - state.lastNonPongRecvMs;
    if (idleSent < cfg.warmThreshMs || idleRecv < cfg.warmThreshMs) {
        return cfg.aggressiveMs;
    }

    const qint64 minIdle = std::min(idleSent, idleRecv);
    if (minIdle < cfg.coolThreshMs) {
        return cfg.lazyMs;
    }
    if (minIdle < cfg.coldThreshMs) {
        return cfg.cooldownMs;
    }
    return cfg.coldMs;
}

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_PINGPACER_H
