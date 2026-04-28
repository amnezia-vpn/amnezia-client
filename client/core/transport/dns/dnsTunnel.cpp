#include "dnsTunnel.h"

#include "dnsPacket_p.h"

#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkDatagram>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSharedPointer>
#include <QSslError>
#include <QSslSocket>
#include <QStringList>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>

namespace amnezia::transport::dns::DnsTunnel
{

using detail::resolveHostAddress;

namespace
{
    constexpr quint16 EDNS0_PAYLOAD_OPTION_CODE = 65001;
    constexpr quint16 EDNS0_CHUNK_REQUEST_CODE = 65002;
    constexpr quint16 EDNS0_CHUNK_RESPONSE_CODE = 65003;

    struct ChunkMeta
    {
        QByteArray chunkId;
        quint16 totalChunks = 0;
        quint16 chunkIndex = 0;
        quint32 totalSize = 0;
    };

    void appendUint16BE(QByteArray &data, quint16 value)
    {
        data.append(static_cast<char>((value >> 8) & 0xFF));
        data.append(static_cast<char>(value & 0xFF));
    }

    QByteArray buildDnsChunkRequest(const QString &queryName, quint16 transactionId,
                                    const QByteArray &chunkId, quint16 chunkIndex)
    {
        QByteArray query;

        appendUint16BE(query, transactionId);
        appendUint16BE(query, 0x0100);
        appendUint16BE(query, 1);
        appendUint16BE(query, 0);
        appendUint16BE(query, 0);
        appendUint16BE(query, 1);

        const QStringList labels = queryName.split('.');
        for (const QString &label : labels) {
            QByteArray labelBytes = label.toUtf8();
            query.append(static_cast<char>(labelBytes.size()));
            query.append(labelBytes);
        }
        query.append(static_cast<char>(0));
        appendUint16BE(query, 16);
        appendUint16BE(query, 1);

        const quint16 optionDataLen = 4 + 18;

        query.append(static_cast<char>(0));
        appendUint16BE(query, 41);
        appendUint16BE(query, 4096);
        query.append(static_cast<char>(0));
        query.append(static_cast<char>(0));
        appendUint16BE(query, 0);
        appendUint16BE(query, optionDataLen);

        appendUint16BE(query, EDNS0_CHUNK_REQUEST_CODE);
        appendUint16BE(query, 18);
        query.append(chunkId.left(16).leftJustified(16, '\0'));
        appendUint16BE(query, chunkIndex);

        return query;
    }

    ChunkMeta parseChunkMeta(const QByteArray &response)
    {
        ChunkMeta meta;

        if (response.size() < 12) return meta;

        const quint8 *data = reinterpret_cast<const quint8 *>(response.constData());

        const quint16 qdCount = (data[4] << 8) | data[5];
        const quint16 anCount = (data[6] << 8) | data[7];
        const quint16 nsCount = (data[8] << 8) | data[9];
        const quint16 arCount = (data[10] << 8) | data[11];

        int pos = 12;

        auto skipDnsName = [&]() -> bool {
            int maxLabels = 128;
            while (pos < response.size() && data[pos] != 0 && maxLabels-- > 0) {
                if ((data[pos] & 0xC0) == 0xC0) {
                    pos += 2;
                    return pos <= response.size();
                }
                const int labelLen = data[pos];
                if (pos + 1 + labelLen > response.size()) return false;
                pos += labelLen + 1;
            }
            if (pos < response.size() && data[pos] == 0) pos++;
            return pos <= response.size();
        };

        for (int i = 0; i < qdCount && pos < response.size(); ++i) {
            if (!skipDnsName()) return meta;
            if (pos + 4 > response.size()) return meta;
            pos += 4;
        }

        for (int i = 0; i < anCount && pos < response.size(); ++i) {
            if (!skipDnsName()) return meta;
            if (pos + 10 > response.size()) return meta;
            const quint16 rdlen = (data[pos + 8] << 8) | data[pos + 9];
            if (pos + 10 + rdlen > response.size()) return meta;
            pos += 10 + rdlen;
        }

        for (int i = 0; i < nsCount && pos < response.size(); ++i) {
            if (!skipDnsName()) return meta;
            if (pos + 10 > response.size()) return meta;
            const quint16 rdlen = (data[pos + 8] << 8) | data[pos + 9];
            if (pos + 10 + rdlen > response.size()) return meta;
            pos += 10 + rdlen;
        }

        for (int i = 0; i < arCount && pos < response.size(); ++i) {
            if (pos < response.size() && data[pos] == 0) {
                pos++;
            } else {
                if (!skipDnsName()) return meta;
            }

            if (pos + 10 > response.size()) return meta;

            const quint16 rtype = (data[pos] << 8) | data[pos + 1];
            const quint16 rdlen = (data[pos + 8] << 8) | data[pos + 9];
            if (pos + 10 + rdlen > response.size()) return meta;
            pos += 10;

            if (rtype == 41 && rdlen > 0) {
                const int optEnd = pos + rdlen;
                while (pos + 4 <= optEnd) {
                    const quint16 optCode = (data[pos] << 8) | data[pos + 1];
                    const quint16 optLen = (data[pos + 2] << 8) | data[pos + 3];
                    pos += 4;

                    if (optCode == EDNS0_CHUNK_RESPONSE_CODE && optLen >= 24) {
                        meta.chunkId = QByteArray(reinterpret_cast<const char *>(data + pos), 16);
                        meta.totalChunks = (data[pos + 16] << 8) | data[pos + 17];
                        meta.chunkIndex = (data[pos + 18] << 8) | data[pos + 19];
                        meta.totalSize = (static_cast<quint32>(data[pos + 20]) << 24)
                                | (static_cast<quint32>(data[pos + 21]) << 16)
                                | (static_cast<quint32>(data[pos + 22]) << 8) | data[pos + 23];
                        return meta;
                    }
                    pos += optLen;
                }
            } else {
                pos += rdlen;
            }
        }

        return meta;
    }

    QByteArray buildDnsTxtQueryWithPayload(const QString &queryName, quint16 transactionId, const QByteArray &payload)
    {
        QByteArray query;

        appendUint16BE(query, transactionId);
        appendUint16BE(query, 0x0100);
        appendUint16BE(query, 1);
        appendUint16BE(query, 0);
        appendUint16BE(query, 0);
        appendUint16BE(query, 1);

        const QStringList labels = queryName.split('.');
        for (const QString &label : labels) {
            QByteArray labelBytes = label.toUtf8();
            query.append(static_cast<char>(labelBytes.size()));
            query.append(labelBytes);
        }
        query.append(static_cast<char>(0));
        appendUint16BE(query, 16);
        appendUint16BE(query, 1);

        const QByteArray payloadBase64 = payload.toBase64();
        const quint16 optionDataLen = 4 + payloadBase64.size();

        query.append(static_cast<char>(0));
        appendUint16BE(query, 41);
        appendUint16BE(query, 4096);
        query.append(static_cast<char>(0));
        query.append(static_cast<char>(0));
        appendUint16BE(query, 0);
        appendUint16BE(query, optionDataLen);

        appendUint16BE(query, EDNS0_PAYLOAD_OPTION_CODE);
        appendUint16BE(query, payloadBase64.size());
        query.append(payloadBase64);

        return query;
    }

    QByteArray parseDnsTxtResponse(const QByteArray &response)
    {
        if (response.size() < 12) {
            return QByteArray();
        }

        const uchar *data = reinterpret_cast<const uchar *>(response.constData());
        int pos = 0;

        pos += 2;
        const quint16 flags = (data[pos] << 8) | data[pos + 1]; pos += 2;
        const quint16 qdCount = (data[pos] << 8) | data[pos + 1]; pos += 2;
        const quint16 anCount = (data[pos] << 8) | data[pos + 1]; pos += 2;
        pos += 2;
        pos += 2;

        if ((flags & 0x8000) == 0) {
            return QByteArray();
        }

        if (anCount > 100 || qdCount > 10) {
            return QByteArray();
        }

        auto skipDnsName = [&]() -> bool {
            int maxLabels = 128;
            while (pos < response.size() && data[pos] != 0 && maxLabels-- > 0) {
                if ((data[pos] & 0xC0) == 0xC0) {
                    pos += 2;
                    return pos <= response.size();
                }
                const int labelLen = data[pos];
                if (pos + 1 + labelLen > response.size()) return false;
                pos += labelLen + 1;
            }
            if (pos < response.size() && data[pos] == 0) pos++;
            return pos <= response.size();
        };

        for (int i = 0; i < qdCount && pos < response.size(); ++i) {
            if (!skipDnsName()) {
                return QByteArray();
            }
            if (pos + 4 > response.size()) return QByteArray();
            pos += 4;
        }

        QByteArray combinedTxt;
        for (int i = 0; i < anCount && pos < response.size(); ++i) {
            if (!skipDnsName()) {
                break;
            }

            if (pos + 10 > response.size()) {
                break;
            }

            const quint16 rtype = (data[pos] << 8) | data[pos + 1]; pos += 2;
            pos += 2; // class
            pos += 4; // ttl
            const quint16 rdlength = (data[pos] << 8) | data[pos + 1]; pos += 2;

            if (pos + rdlength > response.size()) {
                break;
            }

            if (rtype == 16) {
                const int rdEnd = pos + rdlength;
                while (pos < rdEnd && pos < response.size()) {
                    const quint8 txtLen = data[pos++];
                    if (txtLen > 0 && pos + txtLen <= rdEnd && pos + txtLen <= response.size()) {
                        combinedTxt.append(reinterpret_cast<const char *>(data + pos), txtLen);
                        pos += txtLen;
                    } else {
                        break;
                    }
                }
            } else {
                pos += rdlength;
            }
        }

        if (combinedTxt.isEmpty()) {
            return QByteArray();
        }

        return QByteArray::fromBase64(combinedTxt);
    }
} // namespace

QByteArray send(const QByteArray &payload,
                const QString &endpointName,
                const QString &baseDomain,
                const QString &dnsServer,
                DnsProtocol protocol,
                quint16 port,
                int timeoutMsecs,
                const QString &dohEndpoint)
{
    const QString queryName = QStringLiteral("%1.%2").arg(endpointName, baseDomain);

    switch (protocol) {
    case DnsProtocol::Udp:
        return sendOverUdpChunked(payload, queryName, dnsServer, port, timeoutMsecs);
    case DnsProtocol::Tcp:
        return sendOverTcp(payload, queryName, dnsServer, port, timeoutMsecs);
    case DnsProtocol::Tls:
        return sendOverTls(payload, queryName, dnsServer, port, timeoutMsecs);
    case DnsProtocol::Https:
        return sendOverHttps(payload, queryName, dnsServer, port, dohEndpoint, timeoutMsecs);
    case DnsProtocol::Quic:
        return QByteArray();
    }
    return QByteArray();
}

QByteArray sendOverUdp(const QByteArray &payload, const QString &queryName,
                       const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QUdpSocket socket;

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray query = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);

    if (query.isEmpty()) {
        return QByteArray();
    }

    const QHostAddress dnsAddress = resolveHostAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QByteArray();
    }

    const qint64 bytesWritten = socket.writeDatagram(query, dnsAddress, port);
    if (bytesWritten != query.size()) {
        return QByteArray();
    }

    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMsecs) {
        if (socket.waitForReadyRead(qMax(1, timeoutMsecs - static_cast<int>(timer.elapsed())))) {
            while (socket.hasPendingDatagrams()) {
                QNetworkDatagram datagram = socket.receiveDatagram();
                if (datagram.isValid()) {
                    return parseDnsTxtResponse(datagram.data());
                }
            }
        }
    }

    return QByteArray();
}

QByteArray sendOverTcp(const QByteArray &payload, const QString &queryName,
                       const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    qDebug() << "[DNS-TCP] start: queryName=" << queryName << "server=" << dnsServer
             << "port=" << port << "payloadBytes=" << payload.size();
    QTcpSocket socket;

    const QHostAddress dnsAddress = resolveHostAddress(dnsServer);
    if (dnsAddress.isNull()) {
        qWarning() << "[DNS-TCP] failed to resolve" << dnsServer;
        return QByteArray();
    }

    socket.connectToHost(dnsAddress, port);
    if (!socket.waitForConnected(timeoutMsecs)) {
        qWarning() << "[DNS-TCP] connect failed:" << socket.errorString();
        return QByteArray();
    }
    qDebug() << "[DNS-TCP] connected";

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray query = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);

    if (query.isEmpty()) {
        qWarning() << "[DNS-TCP] failed to build DNS query";
        socket.close();
        return QByteArray();
    }
    qDebug() << "[DNS-TCP] built DNS query bytes=" << query.size() << "txid=" << transactionId;
    qDebug() << "[DNS-TCP] query head hex (first 64 bytes):"
             << query.left(64).toHex(' ');
    qDebug() << "[DNS-TCP] query tail hex (last 32 bytes):"
             << query.right(32).toHex(' ');

    quint16 length = qToBigEndian<quint16>(static_cast<quint16>(query.size()));
    QByteArray tcpQuery;
    tcpQuery.append(reinterpret_cast<const char *>(&length), sizeof(quint16));
    tcpQuery.append(query);

    const qint64 bytesWritten = socket.write(tcpQuery);
    qDebug() << "[DNS-TCP] wrote bytes=" << bytesWritten << "/ expected=" << tcpQuery.size();
    if (bytesWritten != tcpQuery.size() || !socket.waitForBytesWritten(timeoutMsecs)) {
        qWarning() << "[DNS-TCP] write failed:" << socket.errorString();
        socket.close();
        return QByteArray();
    }

    QElapsedTimer timer;
    timer.start();

    while (socket.bytesAvailable() < 2) {
        const int remaining = timeoutMsecs - timer.elapsed();
        if (remaining <= 0 || !socket.waitForReadyRead(remaining)) {
            qWarning() << "[DNS-TCP] timeout waiting for response length, socketState="
                       << socket.state() << "err=" << socket.errorString()
                       << "bytesAvailable=" << socket.bytesAvailable();
            socket.close();
            return QByteArray();
        }
    }

    QByteArray lengthBytes = socket.read(2);
    if (lengthBytes.size() != 2) {
        qWarning() << "[DNS-TCP] could not read length prefix";
        socket.close();
        return QByteArray();
    }

    const quint16 responseLength =
            qFromBigEndian<quint16>(*reinterpret_cast<const quint16 *>(lengthBytes.constData()));
    qDebug() << "[DNS-TCP] response length prefix=" << responseLength;

    QByteArray response;
    while (response.size() < responseLength) {
        const int remaining = timeoutMsecs - timer.elapsed();
        if (remaining <= 0) {
            qWarning() << "[DNS-TCP] timeout reading body, got" << response.size() << "/" << responseLength;
            socket.close();
            return QByteArray();
        }

        if (socket.bytesAvailable() > 0) {
            response.append(socket.read(responseLength - response.size()));
        } else if (!socket.waitForReadyRead(remaining)) {
            qWarning() << "[DNS-TCP] timeout in waitForReadyRead, got" << response.size() << "/" << responseLength;
            socket.close();
            return QByteArray();
        }
    }

    qDebug() << "[DNS-TCP] full response read, bytes=" << response.size();
    socket.close();
    QByteArray parsed = parseDnsTxtResponse(response);
    qDebug() << "[DNS-TCP] parsed TXT payload bytes=" << parsed.size();
    return parsed;
}

QByteArray sendOverTls(const QByteArray &payload, const QString &queryName,
                       const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QSslSocket socket;
#ifdef AGW_INSECURE_SSL
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);
    QObject::connect(&socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
                     &socket, [&socket](const QList<QSslError> &errs) {
                         qWarning() << "[DoT] sslErrors (ignored, AGW_INSECURE_SSL=1):" << errs;
                         socket.ignoreSslErrors();
                     });
#else
    socket.setPeerVerifyMode(QSslSocket::VerifyPeer);
#endif

    const QHostAddress dnsAddress = resolveHostAddress(dnsServer);
    if (dnsAddress.isNull()) {
        qWarning() << "[DoT] failed to resolve" << dnsServer;
        return QByteArray();
    }

    socket.connectToHostEncrypted(dnsServer, port);
    if (!socket.waitForEncrypted(timeoutMsecs)) {
        qWarning() << "[DoT] handshake failed:" << socket.errorString();
        return QByteArray();
    }

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray query = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);

    if (query.isEmpty()) {
        socket.close();
        return QByteArray();
    }

    quint16 length = qToBigEndian<quint16>(static_cast<quint16>(query.size()));
    QByteArray tcpQuery;
    tcpQuery.append(reinterpret_cast<const char *>(&length), sizeof(quint16));
    tcpQuery.append(query);

    const qint64 bytesWritten = socket.write(tcpQuery);
    if (bytesWritten != tcpQuery.size() || !socket.waitForBytesWritten(timeoutMsecs)) {
        socket.close();
        return QByteArray();
    }

    QElapsedTimer timer;
    timer.start();

    while (socket.bytesAvailable() < 2) {
        const int remaining = timeoutMsecs - timer.elapsed();
        if (remaining <= 0 || !socket.waitForReadyRead(remaining)) {
            socket.close();
            return QByteArray();
        }
    }

    QByteArray lengthBytes = socket.read(2);
    if (lengthBytes.size() != 2) {
        socket.close();
        return QByteArray();
    }

    const quint16 responseLength =
            qFromBigEndian<quint16>(*reinterpret_cast<const quint16 *>(lengthBytes.constData()));

    QByteArray response;
    while (response.size() < responseLength) {
        const int remaining = timeoutMsecs - timer.elapsed();
        if (remaining <= 0) {
            socket.close();
            return QByteArray();
        }

        if (socket.bytesAvailable() > 0) {
            response.append(socket.read(responseLength - response.size()));
        } else if (!socket.waitForReadyRead(remaining)) {
            socket.close();
            return QByteArray();
        }
    }

    socket.close();
    return parseDnsTxtResponse(response);
}

QByteArray sendOverHttps(const QByteArray &payload, const QString &queryName,
                         const QString &dnsServer, quint16 port, const QString &endpoint, int timeoutMsecs)
{
    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray dnsQuery = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);

    qDebug() << "[DoH] queryName=" << queryName << "payloadBytes=" << payload.size()
             << "dnsQueryBytes=" << dnsQuery.size() << "txid=" << transactionId;

    if (dnsQuery.isEmpty()) {
        qWarning() << "[DoH] failed to build DNS query (payload too big or queryName invalid)";
        return QByteArray();
    }

    const QString scheme = (port == 443) ? QStringLiteral("https") : QStringLiteral("http");
    const QString url = QStringLiteral("%1://%2:%3%4").arg(scheme).arg(dnsServer).arg(port).arg(endpoint);

    qDebug() << "[DoH] POST" << url << "timeoutMs=" << timeoutMsecs;

    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/dns-message");
    request.setRawHeader("Accept", "application/dns-message");
    request.setTransferTimeout(timeoutMsecs);

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, dnsQuery);

    QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                     [reply](const QList<QSslError> &errs) {
                         qWarning() << "[DoH] sslErrors:" << errs;
#ifdef AGW_INSECURE_SSL
                         qWarning() << "[DoH] AGW_INSECURE_SSL=1, ignoring SSL errors";
                         reply->ignoreSslErrors();
#endif
                     });

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer::singleShot(timeoutMsecs, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->isFinished()) {
        qWarning() << "[DoH] timeout after" << timeoutMsecs << "ms, aborting";
        reply->abort();
        reply->deleteLater();
        return QByteArray();
    }

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[DoH] reply error:" << reply->error() << reply->errorString()
                   << "httpStatus=" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        reply->deleteLater();
        return QByteArray();
    }

    QByteArray response = reply->readAll();
    qDebug() << "[DoH] raw HTTP response bytes=" << response.size()
             << "httpStatus=" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    reply->deleteLater();

    if (response.isEmpty()) {
        qWarning() << "[DoH] empty HTTP response body";
        return QByteArray();
    }

    QByteArray parsed = parseDnsTxtResponse(response);
    qDebug() << "[DoH] parsed TXT payload bytes=" << parsed.size();
    return parsed;
}

QByteArray sendOverUdpChunked(const QByteArray &payload, const QString &queryName,
                              const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    qDebug() << "[DNS-UDP] start: queryName=" << queryName << "server=" << dnsServer
             << "port=" << port << "payloadBytes=" << payload.size() << "timeoutMs=" << timeoutMsecs;
    const QHostAddress dnsAddress = resolveHostAddress(dnsServer);
    if (dnsAddress.isNull()) {
        qWarning() << "[DNS-UDP] failed to resolve" << dnsServer;
        return QByteArray();
    }
    qDebug() << "[DNS-UDP] resolved to" << dnsAddress.toString();

    constexpr int MAX_INITIAL_RETRIES = 3;
    constexpr int MAX_CHUNK_RETRIES = 2;
    constexpr int MAX_CONCURRENT_REQUESTS = 5;
    constexpr int BASE_TIMEOUT_MS = 2000;

    auto sendUdpRequestWithTimeout = [&](const QByteArray &query, int requestTimeoutMs) -> QByteArray {
        QUdpSocket socket;
        const qint64 written = socket.writeDatagram(query, dnsAddress, port);
        if (written != query.size()) {
            return QByteArray();
        }

        QElapsedTimer timer;
        timer.start();

        while (timer.elapsed() < requestTimeoutMs) {
            if (socket.waitForReadyRead(qMax(1, requestTimeoutMs - static_cast<int>(timer.elapsed())))) {
                while (socket.hasPendingDatagrams()) {
                    QNetworkDatagram datagram = socket.receiveDatagram();
                    if (datagram.isValid()) {
                        return datagram.data();
                    }
                }
            }
        }

        return QByteArray();
    };

    auto sendWithRetry = [&](const QByteArray &query, int maxRetries) -> QByteArray {
        for (int attempt = 0; attempt < maxRetries; ++attempt) {
            const int timeout = BASE_TIMEOUT_MS * (attempt + 1);
            QByteArray response = sendUdpRequestWithTimeout(query, timeout);
            if (!response.isEmpty()) {
                return response;
            }

            if (attempt < maxRetries - 1) {
                QThread::msleep(timeout / 2);
            }
        }
        return QByteArray();
    };

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray initialQuery = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);

    qDebug() << "[DNS-UDP] initialQuery size=" << initialQuery.size() << "txid=" << transactionId;

    if (initialQuery.isEmpty()) {
        qWarning() << "[DNS-UDP] failed to build initial query (payload too big or queryName invalid)";
        return QByteArray();
    }

    const QByteArray firstResponse = sendWithRetry(initialQuery, MAX_INITIAL_RETRIES);
    qDebug() << "[DNS-UDP] first response size=" << firstResponse.size();

    if (firstResponse.isEmpty()) {
        qWarning() << "[DNS-UDP] no response from server after" << MAX_INITIAL_RETRIES << "retries";
        return QByteArray();
    }

    const ChunkMeta meta = parseChunkMeta(firstResponse);
    const QByteArray firstTxtData = parseDnsTxtResponse(firstResponse);

    qDebug() << "[DNS-UDP] meta totalChunks=" << meta.totalChunks
             << "chunkId=" << meta.chunkId << "firstTxtData size=" << firstTxtData.size();

    if (firstTxtData.isEmpty()) {
        qWarning() << "[DNS-UDP] failed to parse TXT data from first response";
        return QByteArray();
    }

    if (meta.totalChunks <= 1) {
        qDebug() << "[DNS-UDP] single chunk, returning" << firstTxtData.size() << "bytes";
        return firstTxtData;
    }

    QMap<int, QByteArray> chunks;
    chunks[0] = firstTxtData;

    auto requestChunksBatch = [&](const QList<int> &chunkIndices, int batchTimeout) {
        if (chunkIndices.isEmpty()) return;

        QList<QSharedPointer<QUdpSocket>> sockets;
        QMap<QUdpSocket *, int> socketToIndex;

        for (int idx : chunkIndices) {
            if (chunks.contains(idx)) continue;

            const quint16 chunkTxId =
                    static_cast<quint16>((QDateTime::currentMSecsSinceEpoch() + idx) & 0xFFFF);
            const QByteArray chunkQuery =
                    buildDnsChunkRequest(queryName, chunkTxId, meta.chunkId, idx);

            if (chunkQuery.isEmpty()) {
                continue;
            }

            auto socket = QSharedPointer<QUdpSocket>::create();
            socket->writeDatagram(chunkQuery, dnsAddress, port);
            socketToIndex[socket.data()] = idx;
            sockets.append(socket);
        }

        if (sockets.isEmpty()) return;

        QElapsedTimer deadline;
        deadline.start();
        int receivedCount = 0;
        const int expectedCount = sockets.size();

        while (deadline.elapsed() < batchTimeout && receivedCount < expectedCount
               && chunks.size() < meta.totalChunks) {
            for (auto &socket : sockets) {
                if (socket->waitForReadyRead(50)) {
                    while (socket->hasPendingDatagrams()) {
                        QNetworkDatagram datagram = socket->receiveDatagram();
                        if (datagram.isValid()) {
                            const QByteArray chunkTxtData = parseDnsTxtResponse(datagram.data());
                            if (!chunkTxtData.isEmpty()) {
                                const ChunkMeta chunkMeta = parseChunkMeta(datagram.data());
                                const int idx = (chunkMeta.totalChunks > 0)
                                        ? chunkMeta.chunkIndex
                                        : socketToIndex.value(socket.data(), -1);
                                if (idx >= 0 && !chunks.contains(idx)) {
                                    chunks[idx] = chunkTxtData;
                                    receivedCount++;
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    const int totalTimeout = qMax(timeoutMsecs / 2, 5000);
    const int batchTimeout = totalTimeout / (MAX_CHUNK_RETRIES + 1);

    for (int retryRound = 0; retryRound <= MAX_CHUNK_RETRIES; ++retryRound) {
        QList<int> missing;
        for (int i = 1; i < meta.totalChunks; ++i) {
            if (!chunks.contains(i)) {
                missing.append(i);
            }
        }

        if (missing.isEmpty()) {
            break;
        }

        for (int batchStart = 0; batchStart < missing.size(); batchStart += MAX_CONCURRENT_REQUESTS) {
            const QList<int> batch = missing.mid(batchStart, MAX_CONCURRENT_REQUESTS);
            requestChunksBatch(batch, batchTimeout);
        }
    }

    QList<int> finalMissing;
    for (int i = 0; i < meta.totalChunks; ++i) {
        if (!chunks.contains(i)) {
            finalMissing.append(i);
        }
    }

    if (!finalMissing.isEmpty()) {
        return QByteArray();
    }

    QByteArray combined;
    combined.reserve(meta.totalSize > 0 ? meta.totalSize : meta.totalChunks * 500);

    for (int i = 0; i < meta.totalChunks; ++i) {
        combined.append(chunks[i]);
    }

    return combined;
}

} // namespace amnezia::transport::dns::DnsTunnel
