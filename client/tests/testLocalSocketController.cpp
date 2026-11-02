#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTest>
#include <QUuid>

#include "mozilla/localsocketcontroller.h"

/// <summary>Проверяет конфигурацию разделения трафика без подключения к службе VPN.</summary>
class TestLocalSocketController : public QObject
{
    Q_OBJECT

private:
    /// <summary>Получает конфигурацию контроллера через изолированный локальный канал.</summary>
    /// <param name="config">Исходная конфигурация подключения.</param>
    /// <returns>Сообщение активации либо пустой объект при ошибке обмена.</returns>
    QJsonObject captureConfiguration(const QJsonObject &config)
    {
        QLocalServer server;
        const QString channel = QStringLiteral("amnezia-split-test-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!server.listen(channel)) {
            return {};
        }

        LocalSocketController controller;
        QLocalSocket *socket = controller.findChild<QLocalSocket *>();
        if (!socket) {
            return {};
        }

        QObject::disconnect(socket, nullptr, &controller, nullptr);
        socket->connectToServer(channel);
        if (!socket->waitForConnected(3000)
            || (!server.hasPendingConnections() && !server.waitForNewConnection(3000))) {
            return {};
        }

        QLocalSocket *receiver = server.nextPendingConnection();
        controller.activate(config);
        socket->waitForBytesWritten(1000);
        QByteArray message;
        while (!message.contains('\n')) {
            if (receiver->bytesAvailable() == 0 && !receiver->waitForReadyRead(3000)) {
                return {};
            }
            message.append(receiver->readAll());
        }
        return QJsonDocument::fromJson(message.trimmed()).object();
    }

private slots:
    /// <summary>Задаёт варианты режимов, протоколов и исходных маршрутов.</summary>
    void siteRoutes_data()
    {
        QTest::addColumn<QString>("protocol");
        QTest::addColumn<QJsonArray>("allowed");
        QTest::addColumn<int>("mode");
        QTest::addColumn<QJsonArray>("expected");

        const QList<QJsonArray> variants {
            {}, {"0.0.0.0/0"}, {"0.0.0.0/0", "::/0"}, {"::/0", "0.0.0.0/0"},
            {"0.0.0.0/0", "::/0", "0.0.0.0/0"}, {"10.42.0.7/16", "2001:db8:42::7/64"},
            {"0.0.0.0/0", "10.42.0.0/16"}
        };
        for (const QString &protocol : {QStringLiteral("awg"), QStringLiteral("wireguard")}) {
            for (int variant = 0; variant < variants.size(); ++variant) {
                for (const int mode : {0, 1, 2}) {
                    QJsonArray expected;
                    if (mode == 1 && variant != 5) {
                        expected = {"198.51.100.0/24", "203.0.113.7/32"};
                    } else if (variant == 1) {
                        expected = {"0.0.0.0/0"};
                    } else if (variant == 5) {
                        expected = {"10.42.0.0/16", "2001:db8:42::/64"};
                    } else if (variant == 6) {
                        expected = {"0.0.0.0/0", "10.42.0.0/16"};
                    } else {
                        expected = {"0.0.0.0/0", "::/0"};
                    }
                    const QByteArray name = QStringLiteral("%1-профиль-%2-режим-%3")
                        .arg(protocol).arg(variant).arg(mode).toUtf8();
                    QTest::newRow(name.constData()) << protocol << variants[variant] << mode << expected;
                }
            }
        }
    }

    /// <summary>Проверяет префиксы, семейства адресов и исключения для каждого режима.</summary>
    void siteRoutes()
    {
        QFETCH(QString, protocol);
        QFETCH(QJsonArray, allowed);
        QFETCH(int, mode);
        QFETCH(QJsonArray, expected);
        const QJsonArray sites {"198.51.100.42/24", "203.0.113.7"};
        const QJsonObject config {{"protocol", protocol},
            {protocol + "_config_data", QJsonObject {{"allowed_ips", allowed}, {"hostName", "192.0.2.1"}}},
            {"splitTunnelType", mode}, {"splitTunnelSites", sites}};
        const QJsonObject result = captureConfiguration(config);
        QVERIFY2(!result.isEmpty(), "Контроллер должен передать конфигурацию тестовому приёмнику");

        QStringList actualPrefixes;
        for (const QJsonValue &value : result.value("allowedIPAddressRanges").toArray()) {
            const QJsonObject range = value.toObject();
            const QHostAddress address(range.value("address").toString());
            QCOMPARE(range.value("isIpv6").toBool(), address.protocol() == QAbstractSocket::IPv6Protocol);
            actualPrefixes.append(address.toString() + "/" + QString::number(range.value("range").toInt()));
        }
        QStringList expectedPrefixes;
        for (const QJsonValue &value : expected) {
            expectedPrefixes.append(value.toString());
        }
        actualPrefixes.sort();
        expectedPrefixes.sort();
        QCOMPARE(actualPrefixes, expectedPrefixes);

        QJsonArray expectedExclusions {"192.0.2.1"};
        if (mode == 2) {
            for (const QJsonValue &site : sites) {
                expectedExclusions.append(site);
            }
        }
        QCOMPARE(result.value("excludedAddresses").toArray(), expectedExclusions);
    }

    /// <summary>Задаёт сочетания исключений приложений с режимами маршрутизации сайтов.</summary>
    void appExclusions_data()
    {
        QTest::addColumn<QString>("protocol");
        QTest::addColumn<int>("siteMode");
        QTest::addColumn<int>("appMode");
        QTest::addColumn<QJsonArray>("apps");
        for (const QString &protocol : {QStringLiteral("awg"), QStringLiteral("wireguard")}) {
            for (const int siteMode : {0, 1, 2}) {
                for (const int appState : {0, 1, 2}) {
                    const int appMode = appState == 0 ? 0 : 2;
                    const QJsonArray apps = appState == 2
                        ? QJsonArray {"C:/Program Files/Тест/app.exe", "file:///C:/Tools/browser.exe"} : QJsonArray {};
                    const QByteArray name = QStringLiteral("%1-сайты-%2-приложения-%3")
                        .arg(protocol).arg(siteMode).arg(appState).toUtf8();
                    QTest::newRow(name.constData()) << protocol << siteMode << appMode << apps;
                }
            }
        }
    }

    /// <summary>Проверяет независимую передачу исключений приложений и сайтов.</summary>
    void appExclusions()
    {
        QFETCH(QString, protocol);
        QFETCH(int, siteMode);
        QFETCH(int, appMode);
        QFETCH(QJsonArray, apps);
        const QJsonObject config {{"protocol", protocol},
            {protocol + "_config_data", QJsonObject {{"allowed_ips", QJsonArray {"0.0.0.0/0"}}, {"hostName", "192.0.2.1"}}},
            {"splitTunnelType", siteMode}, {"splitTunnelSites", QJsonArray {"198.51.100.0/24"}},
            {"appSplitTunnelType", appMode}, {"splitTunnelApps", apps}};
        const QJsonObject result = captureConfiguration(config);
        QVERIFY2(!result.isEmpty(), "Контроллер должен передать конфигурацию тестовому приёмнику");
        QCOMPARE(result.value("vpnDisabledApps").toArray(), apps);
    }
};

QTEST_GUILESS_MAIN(TestLocalSocketController)
#include "testLocalSocketController.moc"
