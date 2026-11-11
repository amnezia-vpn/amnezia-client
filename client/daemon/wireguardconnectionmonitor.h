#ifndef WIREGUARDCONNECTIONMONITOR_H
#define WIREGUARDCONNECTIONMONITOR_H

#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <algorithm>
#include <limits>
#include <optional>

class WireguardConnectionMonitor
{
public:
    enum class Action {
        None,
        Rebind,
        Recovered
    };

    explicit WireguardConnectionMonitor(const QString &rejectAfterTime = { }, const QString &rekeyTimeout = { })
        : m_keyLifetimeMsec(upperBound(rejectAfterTime, 180) * 1000),
          m_responseTimeoutMsec(std::max<qint64>(30000, 3 * (upperBound(rekeyTimeout, 5) + 1) * 1000)),
          m_maxRetryIntervalMsec(std::max<qint64>(300000, m_responseTimeoutMsec)),
          m_retryIntervalMsec(m_responseTimeoutMsec)
    {
    }

    Action update(qint64 handshake, qint64 rxBytes, qint64 txBytes, qint64 nowMsec)
    {
        if (!m_established) {
            if (handshake <= 0) {
                return Action::None;
            }
            m_established = true;
            m_handshake = handshake;
            m_handshakeObservedAtMsec = nowMsec;
            m_rxBytes = rxBytes;
            m_txBytes = txBytes;
            return Action::None;
        }

        m_unavailableSinceMsec.reset();
        const bool countersReset = rxBytes < m_rxBytes || txBytes < m_txBytes;
        const bool received = !countersReset && rxBytes > m_rxBytes;
        const bool transmitted = !countersReset && txBytes > m_txBytes;
        const bool handshakeChanged = handshake > 0 && handshake != m_handshake;
        m_rxBytes = rxBytes;
        m_txBytes = txBytes;

        if (handshakeChanged) {
            m_handshake = handshake;
            m_handshakeObservedAtMsec = nowMsec;
            m_expiryObserved = false;
        }

        if (received || handshakeChanged) {
            const Action action = m_recovering ? Action::Recovered : Action::None;
            m_unansweredSinceMsec.reset();
            m_recovering = false;
            m_retryIntervalMsec = m_responseTimeoutMsec;
            return action;
        }

        if (m_recovering) {
            return recoveryAction(nowMsec);
        }

        if (countersReset) {
            m_unansweredSinceMsec.reset();
            m_expiryObserved = false;
        }

        if (handshake <= 0) {
            if (!m_unansweredSinceMsec) {
                m_unansweredSinceMsec = nowMsec;
            }
            return recoveryAction(nowMsec);
        }

        if (!elapsed(m_handshakeObservedAtMsec, m_keyLifetimeMsec, nowMsec)) {
            m_unansweredSinceMsec.reset();
            return Action::None;
        }

        if (!m_expiryObserved) {
            m_expiryObserved = true;
            m_unansweredSinceMsec.reset();
            return Action::None;
        }

        if (transmitted && !m_unansweredSinceMsec) {
            m_unansweredSinceMsec = nowMsec;
        }
        return recoveryAction(nowMsec);
    }

    Action unavailable(qint64 nowMsec)
    {
        if (!m_established) {
            return Action::None;
        }
        if (!m_unavailableSinceMsec) {
            m_unavailableSinceMsec = nowMsec;
        }
        return recoveryAction(nowMsec);
    }

    bool isRecovering() const
    {
        return m_recovering;
    }

private:
    static qint64 upperBound(const QString &value, qint64 fallback)
    {
        const QStringList bounds = value.trimmed().split(QLatin1Char('-'));
        if (bounds.isEmpty() || bounds.size() > 2) {
            return fallback;
        }

        quint64 lower = 0;
        quint64 upper = 0;
        for (qsizetype index = 0; index < bounds.size(); ++index) {
            const QString &bound = bounds.at(index);
            if (bound.isEmpty()) {
                return fallback;
            }
            for (QChar digit : bound) {
                if (digit < QLatin1Char('0') || digit > QLatin1Char('9')) {
                    return fallback;
                }
            }
            bool ok = false;
            const quint64 number = bound.toULongLong(&ok);
            if (!ok || number > (std::numeric_limits<quint32>::max)()) {
                return fallback;
            }
            if (index == 0) {
                lower = number;
            }
            upper = number;
        }
        if (upper == 0 || upper < lower) {
            return fallback;
        }
        return static_cast<qint64>(upper);
    }

    static bool elapsed(qint64 start, qint64 duration, qint64 nowMsec)
    {
        return nowMsec >= start
                && static_cast<quint64>(nowMsec) - static_cast<quint64>(start) >= static_cast<quint64>(duration);
    }

    Action recoveryAction(qint64 nowMsec)
    {
        if (m_recovering) {
            if (!elapsed(m_lastRebindAtMsec, m_retryIntervalMsec, nowMsec)) {
                return Action::None;
            }
        } else {
            const bool unanswered =
                    m_unansweredSinceMsec && elapsed(*m_unansweredSinceMsec, m_responseTimeoutMsec, nowMsec);
            const bool unavailable =
                    m_unavailableSinceMsec && elapsed(*m_unavailableSinceMsec, m_responseTimeoutMsec, nowMsec);
            if (!unanswered && !unavailable) {
                return Action::None;
            }
            m_recovering = true;
        }

        m_lastRebindAtMsec = nowMsec;
        m_retryIntervalMsec = (std::min)(m_retryIntervalMsec * 2, m_maxRetryIntervalMsec);
        return Action::Rebind;
    }

    qint64 m_keyLifetimeMsec;
    qint64 m_responseTimeoutMsec;
    qint64 m_maxRetryIntervalMsec;
    qint64 m_retryIntervalMsec;
    qint64 m_handshake = 0;
    qint64 m_handshakeObservedAtMsec = 0;
    qint64 m_rxBytes = 0;
    qint64 m_txBytes = 0;
    qint64 m_lastRebindAtMsec = 0;
    std::optional<qint64> m_unansweredSinceMsec;
    std::optional<qint64> m_unavailableSinceMsec;
    bool m_established = false;
    bool m_expiryObserved = false;
    bool m_recovering = false;
};

#endif
