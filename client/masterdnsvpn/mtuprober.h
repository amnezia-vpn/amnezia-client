// SPDX-License-Identifier: GPL-3.0-or-later
//
// Per-resolver MTU prober — drives the spec §9 upload + download binary
// searches. The state machine is decoupled from the network layer: the
// caller (Session) bridges by encrypting/encoding the payloads we hand
// back via `nextProbe()` and by feeding decoded responses through
// `feedResponse()`. This makes the search math fully unit-testable.
//
// Wire-level details, ported from upstream Go (`internal/client/mtu.go`):
//
//   * Upload probe (PACKET_MTU_UP_REQ) — `[mode 1B][code 4B BE][filler]`,
//     total payload length = the candidate upload MTU. Server echoes
//     back PACKET_MTU_UP_RES with `[code 4B BE][received_size 2B BE]`
//     (6 bytes total).
//   * Download probe (PACKET_MTU_DOWN_REQ) — `[mode 1B][code 4B BE]
//     [effective_size 2B BE][filler]`, total request payload length
//     >= max(7, uploadMTU). Server echoes back PACKET_MTU_DOWN_RES
//     padded to `effective_size` bytes whose head is `[code 4B BE]
//     [effective_size 2B BE]` and the remainder is filler. The reply
//     size matches `effectiveDownloadMTUProbeSize(downloadMTU)` from
//     spec §13(11) (currently `downloadMTU + 0`).
//
// The search loop mirrors `binarySearchMTU` upstream
// (internal/client/mtu.go:1028-1124):
//
//   1. Probe at `high` first; if it passes, return high.
//   2. Probe at `low`; if it fails, fail the whole search.
//   3. Binary search [low+1, high-1], remembering the best ok-ed value.
//
// Each probe gets `cfg.maxRetries + 1` attempts before being declared
// failed. Mirrors upstream's `c.mtuTestRetries` retry loop.

#ifndef MASTERDNSVPN_MTUPROBER_H
#define MASTERDNSVPN_MTUPROBER_H

#include "wireframing.h"

#include <QByteArray>
#include <QObject>

namespace amnezia::masterdnsvpn {

class MtuProber : public QObject
{
    Q_OBJECT

public:
    struct Config {
        // Upload bounds. minUpload floor (10) mirrors upstream's
        // `minUploadMTUFloor`; maxUpload defaults to 150 (operator-tunable).
        int minUpload = 10;
        int maxUpload = 150;

        // Download bounds. minDownload floor (20 = SessionAcceptPayloadSize)
        // mirrors upstream's `minDownloadMTUFloor`.
        int minDownload = 20;
        int maxDownload = 4096;

        // Per-probe timeout + retry budget. `maxRetries` is the number of
        // additional attempts beyond the first one — total attempts =
        // `maxRetries + 1`. Upstream default is `MTU_TEST_RETRIES = 2`,
        // i.e. up to 3 sends per probe size.
        int timeoutMs = 2000;
        int maxRetries = 2;

        // Whether the server should reply base64-encoded (`mtuProbeBase64Reply
        // = 1`) or raw (`mtuProbeRawResponse = 0`). Matches the
        // `BaseEncodeData` knob in upstream config — must agree with the
        // SESSION_INIT byte 0.
        bool baseEncodeReply = false;
    };

    explicit MtuProber(QObject *parent = nullptr);

    // Kick off the search. Emits `nextProbe` synchronously with the first
    // probe; the caller must send it and feed the response back via
    // `feedResponse`. Multiple `start()` calls cancel and restart.
    void start(const Config &cfg);

    // Abandon the in-flight search; emits no terminal signal.
    void cancel();

    // Whether a search is currently in progress.
    bool isRunning() const { return m_state.phase != Phase::Idle && m_state.phase != Phase::Done; }

    // Feed a decoded inner packet of type MtuUpRes / MtuDownRes; anything
    // else is silently ignored. Mismatched challenge codes or sizes count
    // toward the retry budget for the current candidate.
    void feedResponse(PacketType type, const QByteArray &payload);

    // Drive timeouts. The caller (Session) pumps this from its tick. If
    // the outstanding probe's deadline has elapsed, the current attempt
    // is recorded as a failure and either retried or advances the search.
    void tick(qint64 nowMs);

    // Resulting MTU pair from the last completed search. Zero on failure.
    int uploadMtu() const { return m_result.uploadMtu; }
    int downloadMtu() const { return m_result.downloadMtu; }

signals:
    // Caller must wire-encode the (packetType, payload) into a full
    // inner Packet (with SessionID=0xFF, Cookie=0, StreamID/SeqNum/Frag
    // per the catalogue), encrypt + base-encode, and ship via the
    // resolver socket. `isUpload` is purely informational.
    void nextProbe(PacketType packetType, const QByteArray &payload, bool isUpload);

    // Terminal signal. `ok = false` means the search failed (boundary
    // probes never succeeded) — caller should mark this resolver as
    // unusable. `ok = true` carries the best discovered upload + download
    // sizes; the caller is responsible for clamping the global synced
    // MTU across all resolvers.
    void finished(bool ok, int uploadMtu, int downloadMtu);

private:
    enum class Phase {
        Idle,
        Upload,
        Download,
        Done,
    };

    // Stage within the binary-search routine. Mirrors `binarySearchMTU`:
    //   ProbeHigh   — first attempt at `high`; success short-circuits.
    //   ProbeLow    — second attempt at `low`; failure aborts.
    //   BinaryStep  — middle steps, recording best successful candidate.
    enum class Stage {
        ProbeHigh,
        ProbeLow,
        BinaryStep,
    };

    struct State {
        Phase phase = Phase::Idle;
        Stage stage = Stage::ProbeHigh;
        int low = 0;
        int high = 0;
        int left = 0;     // inclusive — next mid lower bound
        int right = 0;    // inclusive — next mid upper bound
        int candidate = 0;
        int best = 0;
        int attempt = 0;
        quint32 challenge = 0;
        qint64 deadlineMs = 0;
    };

    struct Result {
        int uploadMtu = 0;
        int downloadMtu = 0;
    };

    void enterUploadPhase();
    void enterDownloadPhase();
    void issueProbe(qint64 nowMs);
    void onCandidatePassed();
    void onCandidateFailed();
    void advanceSearch();
    void finishOk();
    void finishFailed();
    QByteArray buildProbePayload(int payloadLen, quint32 challenge, int effectiveDownloadSize) const;

    Config m_cfg;
    State m_state;
    Result m_result;
    quint32 m_challengeCounter = 0;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_MTUPROBER_H
