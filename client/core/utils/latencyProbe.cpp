#include "latencyProbe.h"

#include <algorithm>
#include <cstring>
#include <numeric>

#include <QElapsedTimer>
#include <QHostInfo>
#include <QSocketNotifier>
#include <QTimer>

#include "pingsender.h"
#include "pingsenderfactory.h"

#ifdef Q_OS_WIN
    #include <winsock2.h>

    #include <mstcpip.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <fcntl.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

using ServerQuality::Quality;

namespace
{
    constexpr int excellentLatencyMs = 80;
    constexpr int goodLatencyMs = 150;
    constexpr int fairLatencyMs = 300;
    constexpr int unstableJitterMs = 50;

#ifdef Q_OS_WIN
    using NativeSocket = SOCKET;
    constexpr NativeSocket invalidNativeSocket = INVALID_SOCKET;
    constexpr int handshakeInProgress = WSAEWOULDBLOCK;
    constexpr int handshakeRefused = WSAECONNREFUSED;
#else
    using NativeSocket = int;
    constexpr NativeSocket invalidNativeSocket = -1;
    constexpr int handshakeInProgress = EINPROGRESS;
    constexpr int handshakeRefused = ECONNREFUSED;
#endif

    bool icmpSocketsUnavailable = false;

    int lastSocketError()
    {
#ifdef Q_OS_WIN
        return WSAGetLastError();
#else
        return errno;
#endif
    }

    void closeNativeSocket(NativeSocket socket)
    {
#ifdef Q_OS_WIN
        closesocket(socket);
#else
        close(socket);
#endif
    }

    int startTcpHandshake(NativeSocket &socket, const QHostAddress &address, quint16 port)
    {
        sockaddr_storage target { };
        socklen_t targetSize = 0;
        if (address.protocol() == QAbstractSocket::IPv6Protocol) {
            auto &ipv6 = reinterpret_cast<sockaddr_in6 &>(target);
            ipv6.sin6_family = AF_INET6;
            ipv6.sin6_port = htons(port);
            const Q_IPV6ADDR bytes = address.toIPv6Address();
            std::memcpy(&ipv6.sin6_addr, bytes.c, sizeof(bytes.c));
            targetSize = sizeof(ipv6);
        } else {
            auto &ipv4 = reinterpret_cast<sockaddr_in &>(target);
            ipv4.sin_family = AF_INET;
            ipv4.sin_port = htons(port);
            ipv4.sin_addr.s_addr = htonl(address.toIPv4Address());
            targetSize = sizeof(ipv4);
        }

#ifdef Q_OS_WIN
        static const int winsockStatus = [] {
            WSADATA winsockData;
            return WSAStartup(MAKEWORD(2, 2), &winsockData);
        }();
        Q_UNUSED(winsockStatus)
#endif
        socket = ::socket(target.ss_family, SOCK_STREAM, IPPROTO_TCP);
        if (socket == invalidNativeSocket) {
            return lastSocketError();
        }
#ifdef Q_OS_WIN
        u_long nonBlocking = 1;
        ioctlsocket(socket, FIONBIO, &nonBlocking);
        // Windows retransmits the SYN after a refusal, which hides a closed port's answer for about a second.
        TCP_INITIAL_RTO_PARAMETERS initialRto = { TCP_INITIAL_RTO_UNSPECIFIED_RTT,
                                                  TCP_INITIAL_RTO_NO_SYN_RETRANSMISSIONS };
        DWORD bytesReturned = 0;
        WSAIoctl(socket, SIO_TCP_INITIAL_RTO, &initialRto, sizeof(initialRto), nullptr, 0, &bytesReturned, nullptr,
                 nullptr);
#else
        fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
#endif
        if (::connect(socket, reinterpret_cast<const sockaddr *>(&target), targetSize) == 0) {
            return 0;
        }
        return lastSocketError();
    }

    int finishedHandshakeError(NativeSocket socket)
    {
        int error = 0;
        socklen_t errorSize = sizeof(error);
        if (getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &errorSize) != 0) {
            return lastSocketError();
        }
        return error;
    }

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
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    const auto replyTimeFor = [this, elapsedTimer](int handshakeError) -> std::optional<int> {
        const bool hostAnswered =
                handshakeError == 0 || (handshakeError == handshakeRefused && !m_endpoint.isTcpTransport);
        return hostAnswered ? std::optional<int>(static_cast<int>(elapsedTimer.elapsed())) : std::nullopt;
    };

    NativeSocket socket = invalidNativeSocket;
    const int startError = startTcpHandshake(socket, m_address, m_endpoint.port);
    if (startError != handshakeInProgress) {
        if (socket != invalidNativeSocket) {
            closeNativeSocket(socket);
        }
        completeSample(phase, replyTimeFor(startError));
        return;
    }

    auto *notifier = new QSocketNotifier(static_cast<qintptr>(socket), QSocketNotifier::Write, this);
    connect(notifier, &QObject::destroyed, [socket]() { closeNativeSocket(socket); });
    auto *timeoutTimer = new QTimer(notifier);
    timeoutTimer->setSingleShot(true);

    const auto completeWith = [this, phase, notifier, timeoutTimer](std::optional<int> replyTimeMs) {
        timeoutTimer->stop();
        notifier->setEnabled(false);
        notifier->deleteLater();
        completeSample(phase, replyTimeMs);
    };

    connect(notifier, &QSocketNotifier::activated, this,
            [socket, replyTimeFor, completeWith]() { completeWith(replyTimeFor(finishedHandshakeError(socket))); });
    connect(timeoutTimer, &QTimer::timeout, this, [completeWith]() { completeWith(std::nullopt); });

    timeoutTimer->start(sampleTimeoutMs);
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
    LatencyResult result = LatencyResult::fromReplyTimes(m_replyTimesMs, samplesPerProbe);
    if (m_replyTimesMs.isEmpty() && !m_endpoint.isTcpTransport) {
        result.quality = Quality::NoReply;
    }
    finish(result);
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
