#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLibrary>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QUdpSocket>

#include "core/utils/domainRouting.h"

using namespace amnezia;

class TestDomainRouting : public QObject
{
    Q_OBJECT

private slots:
    /// <summary>Задаёт варианты разбора доменов, масок и адресов.</summary>
    void parse_data();
    /// <summary>Проверяет нормализацию и отказ от некорректных правил.</summary>
    void parse();
    /// <summary>Проверяет конфигурацию и обязательную привязку исходящих путей.</summary>
    void configValidation();
    /// <summary>Проверяет принятие производственной конфигурации без создания интерфейса.</summary>
    void tunConfigAccepted();
    /// <summary>Задаёт режимы проверки реального обработчика трафика.</summary>
    void liveRouting_data();
    /// <summary>Проверяет разделение доменов на одном IP в поставляемом обработчике.</summary>
    void liveRouting();
    /// <summary>Останавливает только экземпляр обработчика в тестовом процессе.</summary>
    void cleanup();

private:
    /// <summary>Создаёт параметры изолированного стенда.</summary>
    /// <returns>Параметры с двумя петлевыми исходящими адресами.</returns>
    domainRouting::Options options() const;
    /// <summary>Запускает отдельный экземпляр поставляемой библиотеки.</summary>
    /// <param name="config">Конфигурация локальных тестовых слушателей.</param>
    /// <returns>Пустая строка при успехе либо описание ошибки.</returns>
    QString startCore(const QJsonObject &config);
    /// <summary>Читает ограниченный объём ответа с обработкой локальных событий.</summary>
    /// <param name="socket">Тестовое TCP-соединение.</param>
    /// <param name="count">Минимальная длина ответа.</param>
    /// <returns>Полученные байты либо неполный ответ при тайм-ауте.</returns>
    QByteArray receive(QTcpSocket &socket, int count) const;
    /// <summary>Отправляет HTTP-запрос через локальный SOCKS-обработчик.</summary>
    /// <param name="socksPort">Порт локального обработчика.</param>
    /// <param name="destination">IP или DNS-имя назначения SOCKS.</param>
    /// <param name="port">Порт тестового HTTP-сервера.</param>
    /// <param name="host">Заголовок Host тестового запроса.</param>
    /// <param name="tls">Использовать ClientHello вместо HTTP-запроса.</param>
    /// <returns>Ответ сервера либо пустая строка при ошибке.</returns>
    QByteArray request(quint16 socksPort, const QString &destination, quint16 port, const QString &host,
                       bool tls = false) const;
    /// <summary>Отправляет UDP-пакет через изолированный SOCKS-обработчик.</summary>
    /// <param name="socksPort">Порт обработчика.</param>
    /// <param name="destination">Домен назначения.</param>
    /// <param name="port">Порт тестового UDP-сервера.</param>
    /// <returns>Полезная нагрузка ответа либо пустой массив при ошибке.</returns>
    QByteArray udpRequest(quint16 socksPort, const QString &destination, quint16 port) const;
    /// <summary>Формирует ClientHello с открытым SNI для проверки распознавания имени.</summary>
    /// <param name="domain">Имя SNI.</param>
    /// <returns>Первое сообщение TLS без проведения TLS-сеанса.</returns>
    QByteArray clientHello(const QString &domain) const;
    /// <summary>Формирует DNS-запрос A-записи.</summary>
    /// <param name="domain">Домен тестового запроса.</param>
    /// <returns>DNS-пакет с одним вопросом.</returns>
    QByteArray dnsQuery(const QString &domain) const;

    QLibrary m_library;
    using Configure = char *(*)(char *);
    using Stop = char *(*)();
    using Free = void (*)(void *);
    Stop m_stop = nullptr;
    Free m_free = nullptr;
};

void TestDomainRouting::parse_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("normalized");
    QTest::newRow("exact") << "Example.COM" << "example.com";
    QTest::newRow("url") << "https://Example.com/path?q=1" << "example.com";
    QTest::newRow("ftp-url") << "ftp://Example.com/path" << "example.com";
    QTest::newRow("root-dot") << "example.com." << "example.com";
    QTest::newRow("wildcard") << " *.Example.COM. " << "*.example.com";
    QTest::newRow("idn") << QString::fromUtf8("*.пример.рф") << "*.xn--e1afmkfd.xn--p1ai";
    QTest::newRow("ipv4") << "192.0.2.3" << "192.0.2.3";
    QTest::newRow("ipv4-url") << "https://192.0.2.3/path" << "192.0.2.3";
    QTest::newRow("cidr") << "192.0.2.129/24" << "192.0.2.0/24";
    QTest::newRow("ipv6") << "2001:db8::1234" << "2001:db8::1234";
    QTest::newRow("ipv6-cidr") << "2001:db8::1234/64" << "2001:db8::/64";
    for (const QString &invalid :
         { QString(), QStringLiteral("*"), QStringLiteral("*example.com"), QStringLiteral("api.*.example.com"),
           QStringLiteral("*.*.example.com"), QStringLiteral("*.example.com/path"),
           QStringLiteral("https://*.example.com"), QStringLiteral("example.com.."), QStringLiteral("bad_.example.com"),
           QStringLiteral("-bad.example.com"), QStringLiteral("192.0.2.1/33"), QStringLiteral("192.0.2.1/024"),
           QStringLiteral("192.0.2.1/"), QStringLiteral("regexp:.*"), QStringLiteral("https://user@example.com"),
           QStringLiteral("file://example.com/path"), QStringLiteral("*.127.0.0.1") }) {
        QTest::newRow(qPrintable(QStringLiteral("reject-") + invalid)) << invalid << QString();
    }
}

void TestDomainRouting::parse()
{
    QFETCH(QString, text);
    QFETCH(QString, normalized);
    const std::optional<domainRouting::Rule> rule = domainRouting::parseRule(text);
    QCOMPARE(rule.has_value(), !normalized.isEmpty());
    if (rule) {
        QCOMPARE(rule->value, normalized);
    }
}

domainRouting::Options TestDomainRouting::options() const
{
    QString loopback;
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
#ifdef Q_OS_WIN
            loopback = interface.humanReadableName();
#else
            loopback = interface.name();
#endif
            break;
        }
    }

    domainRouting::Options result;
    result.mode = RouteMode::VpnAllExceptSites;
    result.rules = { QStringLiteral("selected.test"), QStringLiteral("*.wild.test") };
    result.direct = { loopback, QStringLiteral("127.0.0.2"), { QStringLiteral("127.0.0.1") } };
    result.tunnel = { loopback, QStringLiteral("127.0.0.3"), { QStringLiteral("127.0.0.1") } };
    result.socksPort = 41080;
    result.dnsPort = 41053;
    result.socksUser = QStringLiteral("test-user");
    result.socksPassword = QStringLiteral("test-password");
    return result;
}

void TestDomainRouting::configValidation()
{
    domainRouting::Options input = options();
    QString error;
    const QJsonObject config = domainRouting::buildConfig(input, error);
    QVERIFY2(!config.isEmpty(), qPrintable(error));
    QCOMPARE(config.value("outbounds").toArray().first().toObject().value("protocol").toString(), "blackhole");
    const QJsonObject sniffing = config.value("inbounds").toArray().first().toObject().value("sniffing").toObject();
    QVERIFY(sniffing.value("routeOnly").toBool());
    QVERIFY(sniffing.value("destOverride").toArray().contains("fakedns"));
    input.tunnel.sourceAddress.clear();
    QVERIFY(domainRouting::buildConfig(input, error).isEmpty());
    QVERIFY(!error.isEmpty());
    input = options();
    input.direct.interfaceName.clear();
    QVERIFY(domainRouting::buildConfig(input, error).isEmpty());
    input = options();
    input.rules.append("*.bad.*.test");
    QVERIFY(domainRouting::buildConfig(input, error).isEmpty());
    input = options();
    input.rules.clear();
    QVERIFY(!domainRouting::buildConfig(input, error).isEmpty());
    QVERIFY(error.isEmpty());
}

void TestDomainRouting::tunConfigAccepted()
{
    if (qEnvironmentVariableIsEmpty("AMNEZIA_XRAY_LIBRARY")) {
        QSKIP("Для проверки обработчика задайте AMNEZIA_XRAY_LIBRARY.");
    }

    domainRouting::Options input = options();
    input.tunName = QStringLiteral("AmneziaDomainTestNotStarted");
    input.socksPort = 0;
    input.dnsPort = 0;
    input.appsMode = AppsRouteMode::VpnOnlyForwardApps;
    input.apps = { QCoreApplication::applicationFilePath() };
    QString error;
    const QJsonObject config = domainRouting::buildConfig(input, error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const QJsonArray inbounds = config.value("inbounds").toArray();
    QCOMPARE(inbounds.size(), 1);
    QCOMPARE(inbounds.first().toObject().value("protocol").toString(), QStringLiteral("tun"));
    m_library.setFileName(qEnvironmentVariable("AMNEZIA_XRAY_LIBRARY"));
    QVERIFY2(m_library.load(), qPrintable(m_library.errorString()));
    const Configure configure = reinterpret_cast<Configure>(m_library.resolve("amnezia_xray_configure"));
    m_stop = reinterpret_cast<Stop>(m_library.resolve("amnezia_xray_stop"));
    m_free = reinterpret_cast<Free>(m_library.resolve("amnezia_xray_free"));
    QVERIFY(configure && m_stop && m_free);
    QByteArray json = QJsonDocument(config).toJson(QJsonDocument::Compact);
    if (char *failure = configure(json.data())) {
        error = QString::fromUtf8(failure);
        m_free(failure);
    }
    QVERIFY2(error.isEmpty(), qPrintable(error));
    for (const QNetworkInterface &network : QNetworkInterface::allInterfaces()) {
        QVERIFY(network.humanReadableName() != input.tunName);
    }
}

QString TestDomainRouting::startCore(const QJsonObject &config)
{
    m_library.setFileName(qEnvironmentVariable("AMNEZIA_XRAY_LIBRARY"));
    if (!m_library.load()) {
        return m_library.errorString();
    }

    const Configure configure = reinterpret_cast<Configure>(m_library.resolve("amnezia_xray_configure"));
    const Stop start = reinterpret_cast<Stop>(m_library.resolve("amnezia_xray_start"));
    m_stop = reinterpret_cast<Stop>(m_library.resolve("amnezia_xray_stop"));
    m_free = reinterpret_cast<Free>(m_library.resolve("amnezia_xray_free"));
    if (!configure || !start || !m_stop || !m_free) {
        return QStringLiteral("В библиотеке отсутствует требуемый API.");
    }

    QByteArray json = QJsonDocument(config).toJson(QJsonDocument::Compact);
    if (char *error = configure(json.data())) {
        const QString message = QString::fromUtf8(error);
        m_free(error);
        return message;
    }

    if (char *error = start()) {
        const QString message = QString::fromUtf8(error);
        m_free(error);
        return message;
    }

    return { };
}

QByteArray TestDomainRouting::receive(QTcpSocket &socket, int count) const
{
    QByteArray result;
    QElapsedTimer timer;
    timer.start();
    while (result.size() < count && timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
        result += socket.readAll();
        if (socket.state() == QAbstractSocket::UnconnectedState) {
            break;
        }

        QTest::qWait(1);
    }

    return result;
}

QByteArray TestDomainRouting::request(quint16 socksPort, const QString &destination, quint16 port, const QString &host,
                                      bool tls) const
{
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, socksPort);
    if (!socket.waitForConnected(1000)) {
        return { };
    }

    socket.write(QByteArray::fromHex("050102"));
    if (receive(socket, 2) != QByteArray::fromHex("0502")) {
        return { };
    }

    const QByteArray user("test-user");
    const QByteArray password("test-password");
    socket.write(QByteArray(1, '\x01') + char(user.size()) + user + char(password.size()) + password);
    if (receive(socket, 2) != QByteArray::fromHex("0100")) {
        return { };
    }

    QByteArray connect = QByteArray::fromHex("050100");
    const QHostAddress address(destination);
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        const quint32 ip = address.toIPv4Address();
        connect += '\x01';
        connect += char(ip >> 24);
        connect += char(ip >> 16);
        connect += char(ip >> 8);
        connect += char(ip);
    } else {
        const QByteArray name = destination.toLatin1();
        connect += '\x03';
        connect += char(name.size());
        connect += name;
    }

    connect += char(port >> 8);
    connect += char(port);
    socket.write(connect);
    const QByteArray connected = receive(socket, 10);
    if (connected.size() < 2 || connected.at(1) != '\0') {
        return { };
    }

    if (tls) {
        socket.write(clientHello(host));
    } else {
        const QByteArray header = host.isEmpty() ? QByteArray() : "Host: " + host.toUtf8() + "\r\n";
        socket.write("GET / HTTP/1.0\r\n" + header + "Connection: close\r\n\r\n");
    }

    return receive(socket, 1024);
}

QByteArray TestDomainRouting::clientHello(const QString &domain) const
{
    const QByteArray name = domain.toLatin1();
    QByteArray sni;
    sni += char((name.size() + 3) >> 8);
    sni += char(name.size() + 3);
    sni += '\0';
    sni += char(name.size() >> 8);
    sni += char(name.size());
    sni += name;
    QByteArray extensions = QByteArray::fromHex("0000");
    extensions += char(sni.size() >> 8);
    extensions += char(sni.size());
    extensions += sni;
    QByteArray hello = QByteArray::fromHex("0303") + QByteArray(32, '\x01') + QByteArray::fromHex("000002c02f0100");
    hello += char(extensions.size() >> 8);
    hello += char(extensions.size());
    hello += extensions;
    QByteArray handshake = QByteArray::fromHex("0100");
    handshake += char(hello.size() >> 8);
    handshake += char(hello.size());
    handshake += hello;
    QByteArray record = QByteArray::fromHex("160301");
    record += char(handshake.size() >> 8);
    record += char(handshake.size());
    return record + handshake;
}

QByteArray TestDomainRouting::udpRequest(quint16 socksPort, const QString &destination, quint16 port) const
{
    QTcpSocket control;
    control.connectToHost(QHostAddress::LocalHost, socksPort);
    if (!control.waitForConnected(1000)) {
        return { };
    }
    control.write(QByteArray::fromHex("050102"));
    if (receive(control, 2) != QByteArray::fromHex("0502")) {
        return { };
    }
    control.write(QByteArray::fromHex("0109") + "test-user" + QByteArray::fromHex("0d") + "test-password");
    if (receive(control, 2) != QByteArray::fromHex("0100")) {
        return { };
    }
    control.write(QByteArray::fromHex("05030001000000000000"));
    const QByteArray associated = receive(control, 10);
    if (associated.size() != 10 || associated.at(1) != '\0' || associated.at(3) != '\x01') {
        return { };
    }
    const quint16 relayPort = quint16(quint8(associated[8])) << 8 | quint8(associated[9]);
    QUdpSocket socket;
    if (!socket.bind(QHostAddress::LocalHost, 0)) {
        return { };
    }
    const QByteArray name = destination.toLatin1();
    QByteArray packet = QByteArray::fromHex("00000003") + char(name.size()) + name;
    packet += char(port >> 8);
    packet += char(port);
    packet += "probe";
    socket.writeDatagram(packet, QHostAddress::LocalHost, relayPort);
    QElapsedTimer timer;
    timer.start();
    while (!socket.hasPendingDatagrams() && timer.elapsed() < 5000) {
        QTest::qWait(1);
    }
    if (!socket.hasPendingDatagrams()) {
        return { };
    }
    const QByteArray response = socket.receiveDatagram().data();
    if (response.size() < 5) {
        return { };
    }
    const int headerLength = response.at(3) == '\x01' ? 10
            : response.at(3) == '\x04'                ? 22
            : response.at(3) == '\x03'                ? 7 + quint8(response.at(4))
                                                      : response.size();
    return response.mid(headerLength);
}

QByteArray TestDomainRouting::dnsQuery(const QString &domain) const
{
    QByteArray query = QByteArray::fromHex("123401000001000000000000");
    for (const QString &label : domain.split('.')) {
        const QByteArray bytes = label.toLatin1();
        query += char(bytes.size());
        query += bytes;
    }

    query += QByteArray::fromHex("0000010001");
    return query;
}

void TestDomainRouting::liveRouting_data()
{
    QTest::addColumn<bool>("include");
    QTest::addColumn<int>("appsMode");
    QTest::addColumn<bool>("listedApp");
    for (const bool include : { false, true }) {
        for (const AppsRouteMode mode :
             { AppsRouteMode::VpnAllApps, AppsRouteMode::VpnOnlyForwardApps, AppsRouteMode::VpnAllExceptApps }) {
            for (const bool listed : { false, true }) {
                const QByteArray name =
                        QStringLiteral("sites-%1/apps-%2/listed-%3").arg(include).arg(mode).arg(listed).toLatin1();
                QTest::newRow(name.constData()) << include << int(mode) << listed;
            }
        }
    }
}

void TestDomainRouting::liveRouting()
{
    if (qEnvironmentVariableIsEmpty("AMNEZIA_XRAY_LIBRARY")) {
        QSKIP("Для проверки обработчика задайте AMNEZIA_XRAY_LIBRARY.");
    }

    QFETCH(bool, include);
    QFETCH(int, appsMode);
    QFETCH(bool, listedApp);
    QTcpServer http;
    QVERIFY(http.listen(QHostAddress::LocalHost, 0));
    QTcpServer changedHttp;
    QVERIFY(changedHttp.listen(QHostAddress("127.0.0.4"), http.serverPort()));
    for (QTcpServer *server : { &http, &changedHttp }) {
        connect(server, &QTcpServer::newConnection, server, [server]() {
            while (server->hasPendingConnections()) {
                QTcpSocket *socket = server->nextPendingConnection();
                connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
                    const QByteArray data = socket->readAll();
                    if (!data.startsWith("GET ") && !data.startsWith('\x16')) {
                        return;
                    }

                    const QByteArray body =
                            socket->localAddress().toString().toUtf8() + '/' + socket->peerAddress().toString().toUtf8();
                    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
                                  + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    QUdpSocket echo;
    QVERIFY(echo.bind(QHostAddress::LocalHost, 0));
    connect(&echo, &QUdpSocket::readyRead, &echo, [&echo]() {
        while (echo.hasPendingDatagrams()) {
            const QNetworkDatagram datagram = echo.receiveDatagram();
            echo.writeDatagram(datagram.senderAddress().toString().toUtf8(), datagram.senderAddress(),
                               datagram.senderPort());
        }
    });

    QUdpSocket dns;
    QVERIFY(dns.bind(QHostAddress::LocalHost, 0));
    quint8 destinationSuffix = 1;
    bool dnsFailure = false;
    QStringList dnsSources;
    connect(&dns, &QUdpSocket::readyRead, &dns, [&dns, &destinationSuffix, &dnsFailure, &dnsSources]() {
        while (dns.hasPendingDatagrams()) {
            const QNetworkDatagram datagram = dns.receiveDatagram();
            QByteArray response = datagram.data();
            if (response.size() < 17) {
                continue;
            }

            response[2] = char(0x81);
            response[3] = char(dnsFailure ? 0x83 : 0x80);
            const bool cname = response.mid(12).startsWith(QByteArray::fromHex("05616c6961730477696c64047465737400"));
            const bool multiple =
                    response.mid(12).startsWith(QByteArray::fromHex("056d756c74690477696c64047465737400"));
            response[7] = char(dnsFailure ? 0 : (cname || multiple ? 2 : 1));
            if (!dnsFailure) {
                if (cname) {
                    const QByteArray target = QByteArray::fromHex("06746172676574047465737400");
                    response += QByteArray::fromHex("c00c0005000100000001");
                    response += char(target.size() >> 8);
                    response += char(target.size());
                    response += target;
                    response += target + QByteArray::fromHex("000100010000000100047f0000");
                    response += char(destinationSuffix);
                } else {
                    response += QByteArray::fromHex("c00c000100010000000100047f0000");
                    response += char(destinationSuffix);
                    if (multiple) {
                        response += QByteArray::fromHex("c00c000100010000000100047f000004");
                    }
                }
            }
            dnsSources.append(datagram.senderAddress().toString());
            dns.writeDatagram(response, datagram.senderAddress(), datagram.senderPort());
        }
    });

    QTcpServer reserveSocks;
    QVERIFY(reserveSocks.listen(QHostAddress::LocalHost, 0));
    QTcpServer reserveDns;
    QVERIFY(reserveDns.listen(QHostAddress::LocalHost, 0));
    domainRouting::Options input = options();
    input.mode = include ? RouteMode::VpnOnlyForwardSites : RouteMode::VpnAllExceptSites;
    input.rules.append(QStringLiteral("127.0.0.4/32"));
    input.hosts.insert(QStringLiteral("static.wild.test"), QJsonArray { "127.0.0.1" });
    input.appsMode = static_cast<AppsRouteMode>(appsMode);
    input.apps = { listedApp ? QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath()
                             : QCoreApplication::applicationDirPath() + QStringLiteral("/not-this-app.exe") };
    input.socksPort = reserveSocks.serverPort();
    input.dnsPort = reserveDns.serverPort();
    QString error;
    QJsonObject config = domainRouting::buildConfig(input, error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QJsonObject dnsConfig = config.value("dns").toObject();
    QJsonArray servers = dnsConfig.value("servers").toArray();
    for (qsizetype i = 1; i < servers.size(); ++i) {
        QJsonObject server = servers.at(i).toObject();
        server.insert("port", dns.localPort());
        servers.replace(i, server);
    }

    dnsConfig.insert("servers", servers);
    config.insert("dns", dnsConfig);
    reserveSocks.close();
    reserveDns.close();
    error = startCore(config);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const bool directApp = (input.appsMode == AppsRouteMode::VpnAllExceptApps && listedApp)
            || (input.appsMode == AppsRouteMode::VpnOnlyForwardApps && !listedApp);
    const QByteArray selected = include && !directApp ? QByteArray("127.0.0.3") : QByteArray("127.0.0.2");
    const QByteArray fallback = include || directApp ? QByteArray("127.0.0.2") : QByteArray("127.0.0.3");
    for (const QString &name :
         { QStringLiteral("selected.test"), QStringLiteral("api.wild.test"), QStringLiteral("a.b.wild.test") }) {
        const QByteArray reply = request(input.socksPort, QStringLiteral("127.0.0.1"), http.serverPort(), name);
        QVERIFY2(reply.endsWith(selected), qPrintable(name + QString::fromUtf8(reply)));
    }

    for (const QString &name : { QStringLiteral("other.test"), QStringLiteral("wild.test"),
                                 QStringLiteral("notwild.test"), QStringLiteral("api.wild.test.evil.test") }) {
        const QByteArray reply = request(input.socksPort, QStringLiteral("127.0.0.1"), http.serverPort(), name);
        QVERIFY2(reply.endsWith(fallback), qPrintable(name + QString::fromUtf8(reply)));
    }

    const QByteArray tlsSelected = request(input.socksPort, QStringLiteral("127.0.0.1"), http.serverPort(),
                                           QStringLiteral("sni.wild.test"), true);
    QVERIFY2(tlsSelected.endsWith(selected), tlsSelected.constData());
    const QByteArray tlsFallback =
            request(input.socksPort, QStringLiteral("127.0.0.1"), http.serverPort(), QStringLiteral("wild.test"), true);
    QVERIFY2(tlsFallback.endsWith(fallback), tlsFallback.constData());

    QCOMPARE(udpRequest(input.socksPort, QStringLiteral("udp.wild.test"), echo.localPort()), selected);
    QCOMPARE(udpRequest(input.socksPort, QStringLiteral("other.test"), echo.localPort()), fallback);
    const QByteArray cnameReply =
            request(input.socksPort, QStringLiteral("alias.wild.test"), http.serverPort(), QString());
    QVERIFY2(cnameReply.endsWith(selected), cnameReply.constData());
    const QByteArray multiReply =
            request(input.socksPort, QStringLiteral("multi.wild.test"), http.serverPort(), QString());
    QVERIFY2(multiReply.endsWith(selected), multiReply.constData());

    QUdpSocket client;
    QVERIFY(client.bind(QHostAddress::LocalHost, 0));
    client.writeDatagram(dnsQuery(QStringLiteral("api.wild.test")), QHostAddress::LocalHost, input.dnsPort);
    QTRY_VERIFY_WITH_TIMEOUT(client.hasPendingDatagrams(), 5000);
    const QByteArray response = client.receiveDatagram().data();
    QVERIFY(response.size() > 4);
    const QByteArray bytes = response.right(4);
    const quint32 fakeIp = quint32(quint8(bytes[0])) << 24 | quint32(quint8(bytes[1])) << 16
            | quint32(quint8(bytes[2])) << 8 | quint8(bytes[3]);
    QVERIFY(QHostAddress(fakeIp).isInSubnet(QHostAddress("198.18.0.0"), 15));
    const QByteArray reply = request(input.socksPort, QHostAddress(fakeIp).toString(), http.serverPort(), QString());
    QVERIFY2(reply.endsWith(selected), reply.constData());
    QVERIFY2(reply.contains("127.0.0.1/"), reply.constData());
    QVERIFY(dnsSources.contains(include ? QStringLiteral("127.0.0.3") : QStringLiteral("127.0.0.2")));
    destinationSuffix = 4;
    QTest::qWait(1200);
    const QByteArray domainReply = request(input.socksPort, QStringLiteral("api.wild.test"), http.serverPort(),
                                           QStringLiteral("api.wild.test"));
    QVERIFY2(domainReply.endsWith(selected), domainReply.constData());
    QVERIFY2(domainReply.contains("127.0.0.4/"), domainReply.constData());
    const QByteArray fakeAfterChange =
            request(input.socksPort, QHostAddress(fakeIp).toString(), http.serverPort(), QString());
    QVERIFY2(fakeAfterChange.contains("127.0.0.4/") && fakeAfterChange.endsWith(selected), fakeAfterChange.constData());

    const QByteArray cidrReply = request(input.socksPort, QStringLiteral("127.0.0.4"), http.serverPort(), QString());
    QVERIFY2(cidrReply.endsWith(selected), cidrReply.constData());
    const QByteArray hostsReply = request(input.socksPort, QStringLiteral("static.wild.test"), http.serverPort(),
                                          QStringLiteral("static.wild.test"));
    QVERIFY2(hostsReply.contains("127.0.0.1/") && hostsReply.endsWith(selected), hostsReply.constData());

    dnsFailure = true;
    dnsSources.clear();
    QTest::qWait(1200);
    const QByteArray failedReply =
            request(input.socksPort, QStringLiteral("api.wild.test"), http.serverPort(), QString());
    QVERIFY(!failedReply.contains("HTTP/1.1 200"));
    QVERIFY(!dnsSources.isEmpty());
    for (const QString &source : dnsSources) {
        QCOMPARE(source, include ? QStringLiteral("127.0.0.3") : QStringLiteral("127.0.0.2"));
    }
}

void TestDomainRouting::cleanup()
{
    if (m_stop && m_free) {
        if (char *error = m_stop()) {
            m_free(error);
        }
    }
}

QTEST_GUILESS_MAIN(TestDomainRouting)
#include "testDomainRouting.moc"
