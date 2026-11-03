#ifndef XRAY_TEST_RUNTIME_H
#define XRAY_TEST_RUNTIME_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QHostAddress>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QtEndian>

#include <functional>

namespace xrayruntime
{
    inline int freeLocalPort()
    {
        QTcpServer server;
        server.listen(QHostAddress::LocalHost, 0);
        const int port = server.serverPort();
        server.close();
        return port;
    }

    inline QString collapsedLog(const QString &output)
    {
        static const QRegularExpression entry(QStringLiteral(R"(\[(?:Error|Warning)\]\s+(?:\[\d+\]\s+)?(.+)$)"));
        static const QRegularExpression address(QStringLiteral(R"(\d{1,3}(?:\.\d{1,3}){3}(?::\d+)?)"));
        QStringList messages;
        QHash<QString, int> counts;
        for (const QString &line : output.split(QLatin1Char('\n'))) {
            const QRegularExpressionMatch match = entry.match(line.trimmed());
            if (!match.hasMatch())
                continue;
            QString message = match.captured(1);
            message.replace(address, QStringLiteral("<ip>"));
            if (message.startsWith(QLatin1String("core: Xray ")) && message.endsWith(QLatin1String(" started")))
                continue;
            if (!counts.contains(message))
                messages << message;
            ++counts[message];
        }
        QStringList collapsed;
        for (const QString &message : messages)
            collapsed << (counts.value(message) > 1
                                  ? QStringLiteral("%1 x%2").arg(message, QString::number(counts.value(message)))
                                  : message);
        return collapsed.join(QStringLiteral(" | "));
    }

    inline QString failureSummary(const QString &title, const QStringList &failures, int casesRun)
    {
        if (failures.isEmpty())
            return QStringLiteral("%1: all %2 cases passed").arg(title, QString::number(casesRun));
        return QStringLiteral("%1: %2 of %3 cases failed\n  %4")
                .arg(title, QString::number(failures.size()), QString::number(casesRun),
                     failures.join(QStringLiteral("\n  ")));
    }

    inline QString version(const QString &bin)
    {
        QProcess process;
        process.start(bin, { QStringLiteral("version") });
        process.waitForFinished(30000);
        static const QRegularExpression number(QStringLiteral(R"(Xray\s+v?(\d+\.\d+\.\d+))"));
        return number.match(QString::fromUtf8(process.readAllStandardOutput())).captured(1);
    }

    inline bool versionAtLeast(const QString &version, const QString &minimum)
    {
        const QStringList have = version.split(QLatin1Char('.'));
        const QStringList need = minimum.split(QLatin1Char('.'));
        for (int i = 0; i < need.size(); ++i) {
            const int h = i < have.size() ? have.at(i).toInt() : 0;
            const int n = need.at(i).toInt();
            if (h != n)
                return h > n;
        }
        return true;
    }

    inline bool waitUntil(const std::function<bool()> &done, int timeoutMs)
    {
        QElapsedTimer timer;
        timer.start();
        while (!done()) {
            if (timer.elapsed() > timeoutMs)
                return false;
            QTest::qWait(10);
        }
        return true;
    }

    struct XrayRun
    {
        QProcess process;
        QByteArray output;

        bool start(const QString &bin, const QString &configPath, int timeoutMs = 10000)
        {
            process.setProcessChannelMode(QProcess::MergedChannels);
            process.start(bin, { QStringLiteral("run"), QStringLiteral("-c"), configPath });
            if (!process.waitForStarted(timeoutMs))
                return false;
            return waitUntil(
                           [this] {
                               output += process.readAll();
                               return output.contains(" started") || process.state() != QProcess::Running;
                           },
                           timeoutMs)
                    && process.state() == QProcess::Running;
        }

        QString stop()
        {
            process.kill();
            process.waitForFinished(3000);
            output += process.readAll();
            return QString::fromUtf8(output);
        }
    };

    inline QByteArray httpGetThroughSocks(quint16 socksPort, const QHostAddress &target, quint16 targetPort,
                                          const QByteArray &path, int timeoutMs, QString &error)
    {
        QTcpSocket socket;
        QByteArray received;
        auto readAtLeast = [&](int size) {
            return waitUntil(
                           [&] {
                               received += socket.readAll();
                               return received.size() >= size || socket.state() != QAbstractSocket::ConnectedState;
                           },
                           timeoutMs)
                    && received.size() >= size;
        };

        socket.connectToHost(QHostAddress::LocalHost, socksPort);
        if (!waitUntil([&] { return socket.state() == QAbstractSocket::ConnectedState; }, timeoutMs)) {
            error = QStringLiteral("local socks is not reachable");
            return {};
        }
        socket.write(QByteArray::fromHex("050100"));
        if (!readAtLeast(2) || received.left(2) != QByteArray::fromHex("0500")) {
            error = QStringLiteral("socks greeting failed");
            return {};
        }
        received.remove(0, 2);

        QByteArray connect = QByteArray::fromHex("05010001");
        const quint32 ip = qToBigEndian(target.toIPv4Address());
        connect.append(reinterpret_cast<const char *>(&ip), 4);
        const quint16 port = qToBigEndian(targetPort);
        connect.append(reinterpret_cast<const char *>(&port), 2);
        socket.write(connect);
        if (!readAtLeast(10) || received.at(1) != 0) {
            error = QStringLiteral("socks connect failed");
            return {};
        }
        received.remove(0, 10);

        socket.write("GET " + path + " HTTP/1.1\r\nHost: runtime.test\r\nConnection: close\r\n\r\n");
        waitUntil(
                [&] {
                    received += socket.readAll();
                    return received.contains(path.mid(1)) || socket.state() != QAbstractSocket::ConnectedState;
                },
                timeoutMs);
        received += socket.readAll();
        if (!received.contains(path.mid(1)))
            error = received.isEmpty() ? QStringLiteral("no response through the tunnel")
                                       : QStringLiteral("unexpected response through the tunnel");
        return received;
    }
}

#endif
