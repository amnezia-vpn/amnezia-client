#ifndef LATENCYPROBE_H
#define LATENCYPROBE_H

#include <optional>

#include <QHostAddress>
#include <QObject>
#include <QVector>

class QHostInfo;

namespace ServerQuality
{
    Q_NAMESPACE
    enum class Quality {
        Unknown = 0,
        Excellent,
        Good,
        Fair,
        Poor,
        Unreachable,
        NoReply
    };
    Q_ENUM_NS(Quality)
}

struct LatencyEndpoint
{
    QString host;
    quint16 port = 0;
    bool isTcpTransport = false;
};

struct LatencyResult
{
    int latencyMs = -1;
    int jitterMs = 0;
    int lossPercent = 0;
    ServerQuality::Quality quality = ServerQuality::Quality::Unknown;

    static LatencyResult fromReplyTimes(const QVector<int> &replyTimesMs, int sentCount);
};

class LatencyProbe : public QObject
{
    Q_OBJECT

public:
    static constexpr int samplesPerProbe = 4;
    static constexpr int sampleIntervalMs = 200;
    static constexpr int sampleTimeoutMs = 1500;

    explicit LatencyProbe(const LatencyEndpoint &endpoint, QObject *parent = nullptr);

    void start();

signals:
    void finished(const LatencyResult &result);

private:
    enum class Method {
        Icmp,
        Tcp
    };

    void onHostResolved(const QHostInfo &hostInfo);
    void startWithAddress(const QHostAddress &address);
    void startSampling(Method method);
    void sendNextSample();
    void sendIcmpSample();
    void sendTcpSample();
    void completeSample(int phase, std::optional<int> replyTimeMs);
    void onIcmpUnavailable(int phase);
    void finish(const LatencyResult &result);

    LatencyEndpoint m_endpoint;
    QHostAddress m_address;
    Method m_method = Method::Icmp;
    QVector<int> m_replyTimesMs;
    int m_startedSamples = 0;
    int m_completedSamples = 0;
    int m_phase = 0;
};

#endif // LATENCYPROBE_H
