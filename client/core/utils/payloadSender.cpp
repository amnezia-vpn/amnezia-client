#include "payloadSender.h"

#include <QDebug>
#include <QEventLoop>
#include <QHostAddress>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

#include "core/utils/constants/configKeys.h"
#include "core/utils/networkUtilities.h"

using namespace amnezia;

namespace
{
    constexpr char protoUdp[] = "udp";
    constexpr char protoTcp[] = "tcp";

    QByteArray randomBytes(int count)
    {
        if (count <= 0) {
            return {};
        }
        QByteArray bytes(count, Qt::Uninitialized);
        for (int i = 0; i < count; ++i) {
            bytes[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
        }
        return bytes;
    }

    bool parseEndpoint(const QString &endpoint, QString &host, quint16 &port)
    {
        const int separatorIndex = endpoint.lastIndexOf(QLatin1Char(':'));
        if (separatorIndex <= 0 || separatorIndex == endpoint.size() - 1) {
            return false;
        }
        host = endpoint.left(separatorIndex);
        bool ok = false;
        const uint parsedPort = endpoint.mid(separatorIndex + 1).toUInt(&ok);
        if (!ok || parsedPort == 0 || parsedPort > 65535) {
            return false;
        }
        port = static_cast<quint16>(parsedPort);
        return true;
    }

    enum class ExchangeResult { Sent, Matched, Mismatch, Timeout };

    ExchangeResult waitForResponse(QAbstractSocket &socket, const QByteArray &expectedResponse, int timeoutMs)
    {
        if (expectedResponse.isEmpty()) {
            return ExchangeResult::Sent;
        }

        QByteArray response;
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&socket, &QIODevice::readyRead, &loop, [&]() {
            response.append(socket.readAll());
            if (response.size() >= expectedResponse.size()) {
                loop.quit();
            }
        });

        timer.start(timeoutMs);
        loop.exec();

        if (response.isEmpty()) {
            return ExchangeResult::Timeout;
        }
        return response == expectedResponse ? ExchangeResult::Matched : ExchangeResult::Mismatch;
    }
}

QByteArray PayloadSender::buildPayload(const QString &tagString)
{
    QByteArray result;
    if (tagString.isEmpty()) {
        return result;
    }

    static const QRegularExpression tagRegExp(QStringLiteral("<([^>]*)>"));
    QRegularExpressionMatchIterator it = tagRegExp.globalMatch(tagString);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QStringList parts = match.captured(1).simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            continue;
        }

        const QString tag = parts.at(0).toLower();
        if (tag == QLatin1String("b")) {
            if (parts.size() < 2) {
                continue;
            }
            QString hex = parts.at(1);
            if (hex.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
                hex = hex.mid(2);
            }
            result.append(QByteArray::fromHex(hex.toLatin1()));
        } else if (tag == QLatin1String("r")) {
            if (parts.size() < 2) {
                continue;
            }
            bool ok = false;
            const int count = parts.at(1).toInt(&ok);
            if (ok) {
                result.append(randomBytes(count));
            }
        } else {
            qWarning() << "PayloadSender: unknown payload tag" << tag;
        }
    }

    return result;
}

void PayloadSender::sendAll(const QJsonArray &sendPayload)
{
    for (int i = 0; i < sendPayload.size(); ++i) {
        const QJsonValue value = sendPayload.at(i);
        if (!value.isObject()) {
            continue;
        }
        sendEntry(value.toObject(), i);
    }
}

void PayloadSender::sendEntry(const QJsonObject &entry, int index)
{
    const QString endpoint = entry.value(configKey::sendPayloadEndpoint).toString();
    QString host;
    quint16 port = 0;
    if (!parseEndpoint(endpoint, host, port)) {
        qWarning() << "PayloadSender: skipping entry" << index << "- invalid endpoint" << endpoint;
        return;
    }

    const QString protocol = entry.value(configKey::sendPayloadProtocol).toString().toLower();
    const int timeoutMs = entry.value(configKey::sendPayloadTimeoutMs).toInt();
    const QByteArray payload = buildPayload(entry.value(configKey::sendPayloadData).toString());
    const QByteArray expectedResponse = buildPayload(entry.value(configKey::sendPayloadExpectedResponse).toString());

    const QString resolvedHost = NetworkUtilities::getIPAddress(host);
    const QHostAddress hostAddress(resolvedHost.isEmpty() ? host : resolvedHost);

    QUdpSocket udpSocket;
    QTcpSocket tcpSocket;
    QAbstractSocket *socket = nullptr;
    if (protocol == protoUdp) {
        socket = &udpSocket;
        if (udpSocket.writeDatagram(payload, hostAddress, port) < 0) {
            qWarning() << "PayloadSender: entry" << index << "- udp write failed to" << endpoint << udpSocket.errorString();
            return;
        }
    } else if (protocol == protoTcp) {
        socket = &tcpSocket;
        tcpSocket.connectToHost(hostAddress, port);
        if (!tcpSocket.waitForConnected(timeoutMs)) {
            qWarning() << "PayloadSender: entry" << index << "- tcp connect failed to" << endpoint << tcpSocket.errorString();
            return;
        }
        if (tcpSocket.write(payload) != payload.size()) {
            qWarning() << "PayloadSender: entry" << index << "- tcp write failed to" << endpoint << tcpSocket.errorString();
            return;
        }
        tcpSocket.flush();
    } else {
        qWarning() << "PayloadSender: skipping entry" << index << "- unsupported protocol" << protocol;
        return;
    }

    switch (waitForResponse(*socket, expectedResponse, timeoutMs)) {
    case ExchangeResult::Sent:
        qInfo() << "PayloadSender: entry" << index << "-" << protocol << "payload sent to" << endpoint;
        break;
    case ExchangeResult::Matched:
        qInfo() << "PayloadSender: entry" << index << "-" << protocol << "expected response received from" << endpoint;
        break;
    case ExchangeResult::Mismatch:
        qWarning() << "PayloadSender: entry" << index << "-" << protocol << "response mismatch from" << endpoint;
        break;
    case ExchangeResult::Timeout:
        qWarning() << "PayloadSender: entry" << index << "-" << protocol << "response timeout from" << endpoint;
        break;
    }
}
