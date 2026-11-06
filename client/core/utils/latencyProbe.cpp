#include "latencyProbe.h"

#include <numeric>

#include <QElapsedTimer>
#include <QHostInfo>
#include <QNetworkProxy>
#include <QTcpSocket>
#include <QTimer>

#include "pingsender.h"
#include "pingsenderfactory.h"

using ServerQuality::Quality;

namespace
{
    constexpr int excellentLatencyMs = 80;
    constexpr int goodLatencyMs = 150;
    constexpr int fairLatencyMs = 300;
    constexpr int unstableJitterMs = 50;
    constexpr int qtConnectAttemptsPerAddress = 2;

    bool icmpSocketsUnavailable = false;

    quint16 nextIcmpSequence()
    {
        static quint16 sequence = 0;
        return ++sequence;
    }

    Quality qualityForLatency(int latencyMs)
    {
        if (latencyMs < excellentLatencyMs) {
            return Quality::Excellent;
        }
        if (latencyMs < goodLatencyMs) {
            return Quality::Good;
        }
        if (latencyMs < fairLatencyMs) {
            return Quality::Fair;
        }
        return Quality::Poor;
    }

    Quality oneStepWorse(Quality quality)
    {
        switch (quality) {
        case Quality::Excellent: return Quality::Good;
        case Quality::Good: return Quality::Fair;
        default: return Quality::Poor;
        }
    }
}

LatencyResult LatencyResult::fromReplyTimes(const QVector<int> &replyTimesMs, int sentCount)
{
    LatencyResult result;
    if (replyTimesMs.isEmpty()) {
        result.lossPercent = 100;
        result.quality = Quality::Unreachable;
        return result;
    }

    const int totalMs = std::accumulate(replyTimesMs.cbegin(), replyTimesMs.cend(), 0);
    result.latencyMs = qRound(static_cast<double>(totalMs) / replyTimesMs.size());

    int jitterTotalMs = 0;
    for (int i = 1; i < replyTimesMs.size(); ++i) {
        jitterTotalMs += qAbs(replyTimesMs.at(i) - replyTimesMs.at(i - 1));
    }
    if (replyTimesMs.size() > 1) {
        result.jitterMs = qRound(static_cast<double>(jitterTotalMs) / (replyTimesMs.size() - 1));
    }
    result.lossPercent = qRound(100.0 * (sentCount - replyTimesMs.size()) / sentCount);

    result.quality = qualityForLatency(result.latencyMs);
    if (result.lossPercent > 0 || result.jitterMs > unstableJitterMs) {
        result.quality = oneStepWorse(result.quality);
    }
    return result;
}

LatencyProbe::LatencyProbe(const LatencyEndpoint &endpoint, QObject *parent) : QObject(parent), m_endpoint(endpoint)
{
}

void LatencyProbe::start()
{
    const QHostAddress address(m_endpoint.host);
    if (!address.isNull()) {
        startWithAddress(address);
        return;
    }
    QHostInfo::lookupHost(m_endpoint.host, this, &LatencyProbe::onHostResolved);
}

void LatencyProbe::onHostResolved(const QHostInfo &hostInfo)
{
    const QList<QHostAddress> addresses = hostInfo.addresses();
    const auto ipv4Address = std::find_if(addresses.cbegin(), addresses.cend(), [](const QHostAddress &address) {
        return address.protocol() == QAbstractSocket::IPv4Protocol;
    });

    if (ipv4Address != addresses.cend()) {
        startWithAddress(*ipv4Address);
    } else if (!addresses.isEmpty()) {
        startWithAddress(addresses.first());
    } else {
        finish(LatencyResult::fromReplyTimes({ }, samplesPerProbe));
    }
}

void LatencyProbe::startWithAddress(const QHostAddress &address)
{
    m_address = address;
    const bool canUseIcmp = address.protocol() == QAbstractSocket::IPv4Protocol && !icmpSocketsUnavailable;
    const bool canUseTcp = m_endpoint.port != 0;

    if (canUseTcp && (m_endpoint.isTcpTransport || !canUseIcmp)) {
        startSampling(Method::Tcp);
    } else if (canUseIcmp) {
        startSampling(Method::Icmp);
    } else {
        finish(LatencyResult { });
    }
}

void LatencyProbe::startSampling(Method method)
{
    ++m_phase;
    m_method = method;
    m_replyTimesMs.clear();
    m_startedSamples = 0;
    m_completedSamples = 0;
    sendNextSample();
}

void LatencyProbe::sendNextSample()
{
    const int phase = m_phase;
    ++m_startedSamples;

    if (m_method == Method::Icmp) {
        sendIcmpSample();
    } else {
        sendTcpSample();
    }

    if (phase == m_phase && m_startedSamples < samplesPerProbe) {
        QTimer::singleShot(sampleIntervalMs, this, [this, phase]() {
            if (phase == m_phase) {
                sendNextSample();
            }
        });
    }
}

void LatencyProbe::sendIcmpSample()
{
    const int phase = m_phase;
    PingSender *sender = PingSenderFactory::create(QHostAddress(), this);
    if (!sender->isValid()) {
        icmpSocketsUnavailable = true;
        sender->deleteLater();
        onIcmpUnavailable(phase);
        return;
    }

    auto *timeoutTimer = new QTimer(sender);
    timeoutTimer->setSingleShot(true);
    const quint16 sequence = nextIcmpSequence();
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    const auto releaseSender = [this, sender, timeoutTimer]() {
        timeoutTimer->stop();
        sender->disconnect(this);
        sender->deleteLater();
    };

    connect(sender, &PingSender::recvPing, this,
            [this, phase, sequence, elapsedTimer, releaseSender](quint16 replySequence) {
                if (replySequence != sequence) {
                    return;
                }
                releaseSender();
                completeSample(phase, static_cast<int>(elapsedTimer.elapsed()));
            });
    connect(sender, &PingSender::criticalPingError, this, [this, phase, releaseSender]() {
        releaseSender();
        onIcmpUnavailable(phase);
    });
    connect(timeoutTimer, &QTimer::timeout, this, [this, phase, releaseSender]() {
        releaseSender();
        completeSample(phase, std::nullopt);
    });

    timeoutTimer->start(sampleTimeoutMs);
    sender->sendPing(m_address, sequence);
}

void LatencyProbe::sendTcpSample()
{
    const int phase = m_phase;
    auto *socket = new QTcpSocket(this);
    socket->setProxy(QNetworkProxy::NoProxy);
    auto *timeoutTimer = new QTimer(socket);
    timeoutTimer->setSingleShot(true);
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    const auto completeWith = [this, phase, socket, timeoutTimer](std::optional<int> replyTimeMs) {
        timeoutTimer->stop();
        socket->disconnect(this);
        socket->abort();
        socket->deleteLater();
        completeSample(phase, replyTimeMs);
    };

    connect(socket, &QTcpSocket::connected, this,
            [completeWith, elapsedTimer]() { completeWith(static_cast<int>(elapsedTimer.elapsed())); });
    connect(socket, &QTcpSocket::errorOccurred, this,
            [this, completeWith, elapsedTimer](QAbstractSocket::SocketError error) {
                const bool hostAnswered = error == QAbstractSocket::ConnectionRefusedError && !m_endpoint.isTcpTransport;
                // QAbstractSocket retries a refused address once, so a refusal costs two round trips.
                const int replyTimeMs = static_cast<int>(elapsedTimer.elapsed() / qtConnectAttemptsPerAddress);
                completeWith(hostAnswered ? std::optional<int>(replyTimeMs) : std::nullopt);
            });
    connect(timeoutTimer, &QTimer::timeout, this, [completeWith]() { completeWith(std::nullopt); });

    timeoutTimer->start(sampleTimeoutMs);
    socket->connectToHost(m_address, m_endpoint.port);
}

void LatencyProbe::completeSample(int phase, std::optional<int> replyTimeMs)
{
    if (phase != m_phase) {
        return;
    }
    if (replyTimeMs.has_value()) {
        m_replyTimesMs.append(replyTimeMs.value());
    }
    if (++m_completedSamples < samplesPerProbe) {
        return;
    }

    if (m_method == Method::Icmp && m_replyTimesMs.isEmpty() && m_endpoint.port != 0) {
        startSampling(Method::Tcp);
        return;
    }
    finish(LatencyResult::fromReplyTimes(m_replyTimesMs, samplesPerProbe));
}

void LatencyProbe::onIcmpUnavailable(int phase)
{
    if (phase != m_phase) {
        return;
    }
    if (m_endpoint.port != 0) {
        startSampling(Method::Tcp);
    } else {
        finish(LatencyResult { });
    }
}

void LatencyProbe::finish(const LatencyResult &result)
{
    ++m_phase;
    emit finished(result);
}
