// SPDX-License-Identifier: GPL-3.0-or-later

#include "mtuprober.h"

#include <QDateTime>
#include <QtEndian>
#include <algorithm>

namespace amnezia::masterdnsvpn {

namespace {

// Spec §9 / mtuProbeCodeLength upstream. The 4-byte challenge sits between
// the response-mode byte and the rest of the payload.
constexpr int kCodeLen = 4;

// Mode byte values from upstream `internal/client/mtu.go:32-33`.
constexpr quint8 kRawResponseMode    = 0;
constexpr quint8 kBase64ResponseMode = 1;

// Spec §13(11). Currently `MaxHeaderRawSize() - HeaderRawSize(MTU_DOWN_RES)`
// is 0 because both equal 11 in the present catalogue, but the formula is
// kept for future-proofing — if a packet type ever pushes MaxHeaderRawSize
// above 11, this becomes a non-trivial reserve.
inline int effectiveDownloadProbeSize(int downloadMtu)
{
    // Header sizes are static for now; recompute via wireframing once an
    // exposed `MaxHeaderRawSize()` helper exists. Today both sides are 11.
    constexpr int kReserve = 0;
    return downloadMtu + kReserve;
}

} // namespace

MtuProber::MtuProber(QObject *parent) : QObject(parent) {}

void MtuProber::start(const Config &cfg)
{
    // Restart-from-scratch semantics: drop any in-flight state, reset
    // results, and step into the upload phase.
    m_cfg = cfg;
    m_state = State{};
    m_result = Result{};

    if (m_cfg.maxRetries < 0) m_cfg.maxRetries = 0;
    if (m_cfg.timeoutMs <= 0) m_cfg.timeoutMs = 1;
    if (m_cfg.minUpload < 1)  m_cfg.minUpload = 1;
    if (m_cfg.maxUpload < m_cfg.minUpload) {
        finishFailed();
        return;
    }
    if (m_cfg.minDownload < 1) m_cfg.minDownload = 1;
    if (m_cfg.maxDownload < m_cfg.minDownload) {
        finishFailed();
        return;
    }

    enterUploadPhase();
}

void MtuProber::cancel()
{
    m_state = State{};
    m_state.phase = Phase::Idle;
}

void MtuProber::feedResponse(PacketType type, const QByteArray &payload)
{
    if (!isRunning()) {
        return;
    }

    const bool isUpload = (m_state.phase == Phase::Upload);
    const PacketType expected = isUpload ? PacketType::MtuUpRes : PacketType::MtuDownRes;
    if (type != expected) {
        return;
    }

    if (isUpload) {
        // Upstream `sendUploadMTUProbe` (internal/client/mtu.go:1189-1211):
        //   * payload size must be exactly 6
        //   * payload[0..3] must equal the outstanding challenge
        //   * payload[4..5] must equal the candidate size as uint16 BE
        if (payload.size() != 6) {
            onCandidateFailed();
            return;
        }
        const quint32 echoCode = qFromBigEndian<quint32>(payload.constData());
        if (echoCode != m_state.challenge) {
            onCandidateFailed();
            return;
        }
        const int echoSize = qFromBigEndian<quint16>(payload.constData() + kCodeLen);
        if (echoSize != m_state.candidate) {
            onCandidateFailed();
            return;
        }
        onCandidatePassed();
        return;
    }

    // Download response. Upstream `sendDownloadMTUProbe` (mtu.go:1298-1346):
    //   * total payload size must equal effectiveDownloadProbeSize(cand)
    //   * payload[0..3] = challenge echo
    //   * payload[4..5] = uint16 BE echo of effective_size
    //   * remainder is filler (un-checked content)
    const int effective = effectiveDownloadProbeSize(m_state.candidate);
    if (payload.size() != effective) {
        onCandidateFailed();
        return;
    }
    if (payload.size() < kCodeLen + 2) {
        onCandidateFailed();
        return;
    }
    const quint32 echoCode = qFromBigEndian<quint32>(payload.constData());
    if (echoCode != m_state.challenge) {
        onCandidateFailed();
        return;
    }
    const int echoSize = qFromBigEndian<quint16>(payload.constData() + kCodeLen);
    if (echoSize != effective) {
        onCandidateFailed();
        return;
    }
    onCandidatePassed();
}

void MtuProber::tick(qint64 nowMs)
{
    if (!isRunning()) {
        return;
    }
    if (m_state.candidate == 0) {
        // No probe outstanding (shouldn't happen but be defensive).
        return;
    }
    if (nowMs < m_state.deadlineMs) {
        return;
    }
    onCandidateFailed();
}

// ---------------------------------------------------------------------------
// Phase transitions
// ---------------------------------------------------------------------------

void MtuProber::enterUploadPhase()
{
    m_state.phase = Phase::Upload;
    m_state.stage = Stage::ProbeHigh;
    m_state.low = m_cfg.minUpload;
    m_state.high = m_cfg.maxUpload;
    m_state.best = 0;
    m_state.candidate = m_state.high;
    m_state.attempt = 0;
    m_state.left = m_state.low + 1;
    m_state.right = m_state.high - 1;
    issueProbe(QDateTime::currentMSecsSinceEpoch());
}

void MtuProber::enterDownloadPhase()
{
    m_state.phase = Phase::Download;
    m_state.stage = Stage::ProbeHigh;
    m_state.low = m_cfg.minDownload;
    m_state.high = m_cfg.maxDownload;
    m_state.best = 0;
    m_state.candidate = m_state.high;
    m_state.attempt = 0;
    m_state.left = m_state.low + 1;
    m_state.right = m_state.high - 1;
    issueProbe(QDateTime::currentMSecsSinceEpoch());
}

void MtuProber::issueProbe(qint64 nowMs)
{
    // Mint a fresh challenge per send. Upstream uses an atomic uint32
    // counter (mtu.go:1445); reusing the counter across attempts at the
    // same size would let a slow late reply masquerade as an in-flight
    // confirmation. Bumping per-attempt avoids that.
    ++m_challengeCounter;
    m_state.challenge = m_challengeCounter;
    m_state.deadlineMs = nowMs + m_cfg.timeoutMs;

    const bool isUpload = (m_state.phase == Phase::Upload);
    const PacketType type = isUpload ? PacketType::MtuUpReq : PacketType::MtuDownReq;

    // For uploads the wire payload size equals the candidate (we're
    // probing whether the server can receive that much). For downloads
    // the request payload size is `max(7, uploadMTU)` per upstream
    // (mtu.go:1256), and we ask the server to send back
    // `effectiveDownloadProbeSize(candidate)` bytes.
    int payloadLen = 0;
    int effectiveDown = 0;
    if (isUpload) {
        payloadLen = m_state.candidate;
    } else {
        payloadLen = std::max(1 + kCodeLen + 2, m_result.uploadMtu);
        effectiveDown = effectiveDownloadProbeSize(m_state.candidate);
    }

    QByteArray payload = buildProbePayload(payloadLen, m_state.challenge, effectiveDown);
    emit nextProbe(type, payload, isUpload);
}

void MtuProber::onCandidatePassed()
{
    // Capture as best and continue the search downward / upward.
    m_state.best = m_state.candidate;
    advanceSearch();
}

void MtuProber::onCandidateFailed()
{
    // Retry budget per candidate (upstream mtuTestRetries).
    if (m_state.attempt < m_cfg.maxRetries) {
        ++m_state.attempt;
        issueProbe(QDateTime::currentMSecsSinceEpoch());
        return;
    }
    // Exhausted retries — record as failed and move the search forward.
    m_state.attempt = 0;
    switch (m_state.stage) {
    case Stage::ProbeHigh:
        if (m_state.low == m_state.high) {
            // Only one candidate ever existed and it failed.
            if (m_state.phase == Phase::Upload) {
                finishFailed();
                return;
            }
            finishFailed();
            return;
        }
        // Fall through to ProbeLow.
        m_state.stage = Stage::ProbeLow;
        m_state.candidate = m_state.low;
        issueProbe(QDateTime::currentMSecsSinceEpoch());
        return;
    case Stage::ProbeLow:
        // Both boundaries failed; abandon the search.
        finishFailed();
        return;
    case Stage::BinaryStep:
        // mid failed → search lower half.
        m_state.right = m_state.candidate - 1;
        advanceSearch();
        return;
    }
}

void MtuProber::advanceSearch()
{
    // What was just probed?
    switch (m_state.stage) {
    case Stage::ProbeHigh:
        // High succeeded → done with this phase (use high as best).
        m_result.uploadMtu = (m_state.phase == Phase::Upload) ? m_state.best : m_result.uploadMtu;
        m_result.downloadMtu = (m_state.phase == Phase::Download) ? m_state.best : m_result.downloadMtu;
        if (m_state.phase == Phase::Upload) {
            enterDownloadPhase();
        } else {
            finishOk();
        }
        return;
    case Stage::ProbeLow:
        // Low succeeded → step into the binary search proper.
        m_state.stage = Stage::BinaryStep;
        break;
    case Stage::BinaryStep:
        // mid succeeded → search upper half.
        m_state.left = m_state.candidate + 1;
        break;
    }

    if (m_state.left > m_state.right) {
        // Search exhausted — `best` is final.
        m_result.uploadMtu = (m_state.phase == Phase::Upload) ? m_state.best : m_result.uploadMtu;
        m_result.downloadMtu = (m_state.phase == Phase::Download) ? m_state.best : m_result.downloadMtu;
        if (m_state.phase == Phase::Upload) {
            enterDownloadPhase();
        } else {
            finishOk();
        }
        return;
    }

    m_state.candidate = (m_state.left + m_state.right) / 2;
    m_state.attempt = 0;
    issueProbe(QDateTime::currentMSecsSinceEpoch());
}

void MtuProber::finishOk()
{
    m_state.phase = Phase::Done;
    emit finished(true, m_result.uploadMtu, m_result.downloadMtu);
}

void MtuProber::finishFailed()
{
    m_state.phase = Phase::Done;
    m_result = Result{};
    emit finished(false, 0, 0);
}

QByteArray MtuProber::buildProbePayload(int payloadLen, quint32 challenge, int effectiveDownloadSize) const
{
    QByteArray payload(std::max(payloadLen, 1 + kCodeLen), '\0');
    payload[0] = static_cast<char>(m_cfg.baseEncodeReply ? kBase64ResponseMode : kRawResponseMode);
    qToBigEndian<quint32>(challenge, payload.data() + 1);

    // For download probes, payload[1+kCodeLen..1+kCodeLen+2] holds the
    // requested effective response size as uint16 BE. Upstream writes
    // this AFTER building the random-mode header (mtu.go:1262).
    if (effectiveDownloadSize > 0 && payload.size() >= 1 + kCodeLen + 2) {
        qToBigEndian<quint16>(static_cast<quint16>(effectiveDownloadSize),
                              payload.data() + 1 + kCodeLen);
    }
    return payload;
}

} // namespace amnezia::masterdnsvpn
