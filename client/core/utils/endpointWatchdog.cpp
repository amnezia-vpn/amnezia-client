#include "endpointWatchdog.h"

#include <algorithm>

EndpointWatchdog::EndpointWatchdog(int64_t firstRetryMs, int64_t maxRetryMs)
    : m_firstRetryMs(firstRetryMs), m_maxRetryMs(std::max(firstRetryMs, maxRetryMs)), m_retryMs(firstRetryMs)
{
}

void EndpointWatchdog::tunnelStarted(int64_t nowMs)
{
    m_tunnelStartedMs = nowMs;
    m_lastHandshakeMs = 0;
    m_lastTxGrowthMs = 0;
    m_lastTxBytes = 0;
    m_haveTx = false;
}

bool EndpointWatchdog::observe(int64_t nowMs, int64_t lastHandshakeMs, uint64_t txBytes)
{
    if (m_tunnelStartedMs == 0) {
        m_tunnelStartedMs = nowMs;
    }

    if (lastHandshakeMs > m_lastHandshakeMs) {
        m_lastHandshakeMs = lastHandshakeMs;
        resetRetry();
    }

    if (m_haveTx && txBytes > m_lastTxBytes) {
        m_lastTxGrowthMs = nowMs;
    }
    m_lastTxBytes = txBytes;
    m_haveTx = true;

    const int64_t since = m_lastHandshakeMs > 0 ? m_lastHandshakeMs : m_tunnelStartedMs;
    const bool noHandshake = nowMs - since > kStaleHandshakeMs;
    const bool sending = m_lastTxGrowthMs > 0 && nowMs - m_lastTxGrowthMs <= kStaleHandshakeMs;

    return noHandshake && sending && nowMs >= m_nextRefreshMs;
}

void EndpointWatchdog::refreshAttempted(int64_t nowMs)
{
    m_nextRefreshMs = nowMs + m_retryMs;
    m_retryMs = std::min(m_retryMs * 2, m_maxRetryMs);
}

void EndpointWatchdog::resetRetry()
{
    m_retryMs = m_firstRetryMs;
    m_nextRefreshMs = 0;
}
