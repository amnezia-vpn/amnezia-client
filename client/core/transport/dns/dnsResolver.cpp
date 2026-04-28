#include "dnsResolver.h"

#include "dnsPacket_p.h"

#include <QDateTime>
#include <QEventLoop>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkDatagram>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>

namespace amnezia::transport::dns::DnsResolver
{

using detail::buildDnsQuery;
using detail::parseDnsResponse;
using detail::resolveHostAddress;

QString resolve(const QString &hostname,
                const QString &dnsServer,
                DnsProtocol protocol,
                quint16 port,
                int timeoutMsecs,
                const QString &dohEndpoint)
{
    switch (protocol) {
    case DnsProtocol::Udp:
        return resolveOverUdp(hostname, dnsServer, port, timeoutMsecs);
    case DnsProtocol::Tcp:
        return resolveOverTcp(hostname, dnsServer, port, timeoutMsecs);
    case DnsProtocol::Tls:
        return resolveOverTls(hostname, dnsServer, port, timeoutMsecs);
    case DnsProtocol::Https:
        return resolveOverHttps(hostname, dnsServer, dohEndpoint, timeoutMsecs);
    case DnsProtocol::Quic:
        return resolveOverQuic(hostname, dnsServer, port, timeoutMsecs);
    }
    return QString();
}

QString resolveOverUdp(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QUdpSocket socket;

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        return QString();
    }

    const QHostAddress dnsAddress = resolveHostAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QString();
    }

    const qint64 bytesWritten = socket.writeDatagram(query, dnsAddress, port);
    if (bytesWritten != query.size()) {
        return QString();
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);

    QByteArray response;
    bool responseReceived = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QUdpSocket::readyRead, [&]() {
        while (socket.hasPendingDatagrams()) {
            QNetworkDatagram datagram = socket.receiveDatagram();
            if (datagram.isValid()) {
                response = datagram.data();
                responseReceived = true;
                loop.quit();
            }
        }
    });

    timer.start();
    loop.exec();
    timer.stop();

    if (!responseReceived || response.isEmpty()) {
        return QString();
    }

    return parseDnsResponse(response, false);
}

QString resolveOverTcp(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QTcpSocket socket;

    const QHostAddress dnsAddress = resolveHostAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QString();
    }

    socket.connectToHost(dnsAddress, port);
    if (!socket.waitForConnected(timeoutMsecs)) {
        return QString();
    }

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        socket.close();
        return QString();
    }

    quint16 length = qToBigEndian<quint16>(static_cast<quint16>(query.size()));
    QByteArray tcpQuery;
    tcpQuery.append(reinterpret_cast<const char *>(&length), sizeof(quint16));
    tcpQuery.append(query);

    const qint64 bytesWritten = socket.write(tcpQuery);
    if (bytesWritten != tcpQuery.size() || !socket.waitForBytesWritten(timeoutMsecs)) {
        socket.close();
        return QString();
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);

    QByteArray response;
    bool responseReceived = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::readyRead, [&]() {
        if (socket.bytesAvailable() >= 2 && response.isEmpty()) {
            QByteArray lengthBytes = socket.read(2);
            if (lengthBytes.size() == 2) {
                const quint16 responseLength =
                        qFromBigEndian<quint16>(*reinterpret_cast<const quint16 *>(lengthBytes.constData()));
                while (socket.bytesAvailable() < responseLength) {
                    if (!socket.waitForReadyRead(timeoutMsecs / 2)) {
                        break;
                    }
                }
                if (socket.bytesAvailable() >= responseLength) {
                    response = socket.read(responseLength);
                    responseReceived = true;
                    loop.quit();
                }
            }
        }
    });

    timer.start();
    loop.exec();
    timer.stop();

    socket.close();

    if (!responseReceived || response.isEmpty()) {
        return QString();
    }

    return parseDnsResponse(response, true);
}

QString resolveOverTls(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QSslSocket socket;

    const QHostAddress dnsAddress = resolveHostAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QString();
    }

    socket.setPeerVerifyMode(QSslSocket::QueryPeer);
    socket.connectToHostEncrypted(dnsAddress.toString(), port);

    if (!socket.waitForConnected(timeoutMsecs)) {
        return QString();
    }

    if (!socket.waitForEncrypted(timeoutMsecs)) {
        socket.close();
        return QString();
    }

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        socket.close();
        return QString();
    }

    quint16 length = qToBigEndian<quint16>(static_cast<quint16>(query.size()));
    QByteArray tlsQuery;
    tlsQuery.append(reinterpret_cast<const char *>(&length), sizeof(quint16));
    tlsQuery.append(query);

    const qint64 bytesWritten = socket.write(tlsQuery);
    if (bytesWritten != tlsQuery.size() || !socket.waitForBytesWritten(timeoutMsecs)) {
        socket.close();
        return QString();
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);

    QByteArray response;
    bool responseReceived = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QSslSocket::readyRead, [&]() {
        if (socket.bytesAvailable() >= 2 && response.isEmpty()) {
            QByteArray lengthBytes = socket.read(2);
            if (lengthBytes.size() == 2) {
                const quint16 responseLength =
                        qFromBigEndian<quint16>(*reinterpret_cast<const quint16 *>(lengthBytes.constData()));
                while (socket.bytesAvailable() < responseLength) {
                    if (!socket.waitForReadyRead(timeoutMsecs / 2)) {
                        break;
                    }
                }
                if (socket.bytesAvailable() >= responseLength) {
                    response = socket.read(responseLength);
                    responseReceived = true;
                    loop.quit();
                }
            }
        }
    });

    timer.start();
    loop.exec();
    timer.stop();

    socket.close();

    if (!responseReceived || response.isEmpty()) {
        return QString();
    }

    return parseDnsResponse(response, true);
}

QString resolveOverHttps(const QString &hostname, const QString &dnsServer, const QString &endpoint, int timeoutMsecs)
{
    const QString dohUrl = QStringLiteral("https://%1%2").arg(dnsServer, endpoint);

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        return QString();
    }

    QNetworkRequest request;
    request.setUrl(QUrl(dohUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/dns-message");
    request.setRawHeader("Accept", "application/dns-message");
    request.setTransferTimeout(timeoutMsecs);

    QNetworkAccessManager nam;
    QNetworkReply *reply = nam.post(request, query);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);

    QByteArray response;
    bool responseReceived = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, [&]() {
        if (reply->error() == QNetworkReply::NoError) {
            response = reply->readAll();
            responseReceived = true;
        }
        loop.quit();
    });

    timer.start();
    loop.exec();
    timer.stop();

    reply->deleteLater();

    if (!responseReceived || response.isEmpty()) {
        return QString();
    }

    return parseDnsResponse(response, false);
}

QString resolveOverQuic(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    // QUIC требует специальной библиотеки — пока используем UDP fallback
    QUdpSocket socket;

    const QHostAddress dnsAddress = resolveHostAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QString();
    }

    const quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    const QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        return QString();
    }

    const qint64 bytesWritten = socket.writeDatagram(query, dnsAddress, port);
    if (bytesWritten != query.size()) {
        return QString();
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);

    QByteArray response;
    bool responseReceived = false;

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QUdpSocket::readyRead, [&]() {
        while (socket.hasPendingDatagrams()) {
            QNetworkDatagram datagram = socket.receiveDatagram();
            if (datagram.isValid()) {
                response = datagram.data();
                responseReceived = true;
                loop.quit();
            }
        }
    });

    timer.start();
    loop.exec();
    timer.stop();

    if (!responseReceived || response.isEmpty()) {
        return QString();
    }

    return parseDnsResponse(response, false);
}

} // namespace amnezia::transport::dns::DnsResolver
