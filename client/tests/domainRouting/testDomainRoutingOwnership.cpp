#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QTest>

#include "core/utils/networkUtilities.h"
#include "xray.h"

/// <summary>Выбирает петлевой интерфейс для изолированного тестового процесса.</summary>
/// <returns>Петлевой интерфейс без обращения к системным маршрутам.</returns>
QPair<QString, QNetworkInterface> NetworkUtilities::getGatewayAndIface()
{
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            return { QStringLiteral("127.0.0.1"), interface };
        }
    }
    return {};
}

class TestDomainRoutingOwnership : public QObject
{
    Q_OBJECT

private slots:
    /// <summary>Проверяет защиту обработчика от замены другим потребителем.</summary>
    void exclusiveOwnership();
    /// <summary>Проверяет освобождение ресурсов после некорректной конфигурации.</summary>
    void failedStart();
    /// <summary>Проверяет остановку обработчика после потери владельца.</summary>
    void expiredOwner();
    /// <summary>Останавливает обработчик только в тестовом процессе.</summary>
    void cleanup();

private:
    /// <summary>Создаёт конфигурацию без слушателей и сетевых соединений.</summary>
    /// <returns>Конфигурация изолированного обработчика.</returns>
    QString configuration() const;
};

QString TestDomainRoutingOwnership::configuration() const
{
    return QString::fromUtf8(QJsonDocument(QJsonObject {
        { "log", QJsonObject { { "loglevel", "none" } } },
        { "inbounds", QJsonArray() },
        { "outbounds", QJsonArray { QJsonObject { { "protocol", "blackhole" } } } }
    }).toJson(QJsonDocument::Compact));
}

void TestDomainRoutingOwnership::exclusiveOwnership()
{
    Xray &core = Xray::getInstance();
    const qint64 first = core.start(configuration(), true);
    QVERIFY(first != 0);
    QCOMPARE(core.start(configuration()), 0);
    QCOMPARE(core.start(configuration(), true), 0);
    QVERIFY(core.stop(first + 1));
    QCOMPARE(core.currentToken(), first);
    QVERIFY(!core.keepAlive(first + 1));
    QVERIFY(core.keepAlive(first));
    QVERIFY(core.stop(first));
    QCOMPARE(core.currentToken(), 0);

    const qint64 second = core.start(configuration());
    QVERIFY(second > first);
    QVERIFY(!core.keepAlive(second));
    QCOMPARE(core.start(configuration(), true), 0);
    QCOMPARE(core.currentToken(), second);
    const qint64 third = core.start(configuration());
    QVERIFY(third > second);
    QVERIFY(core.stop(second));
    QCOMPARE(core.currentToken(), third);
}

void TestDomainRoutingOwnership::failedStart()
{
    Xray &core = Xray::getInstance();
    QCOMPARE(core.start(QStringLiteral("{invalid"), true), 0);
    QCOMPARE(core.currentToken(), 0);
    QVERIFY(core.start(configuration(), true) != 0);
}

void TestDomainRoutingOwnership::expiredOwner()
{
    Xray &core = Xray::getInstance();
    const qint64 token = core.start(configuration(), true);
    QVERIFY(token != 0);
    QVERIFY(core.keepAlive(token));
    QTRY_COMPARE_WITH_TIMEOUT(core.currentToken(), 0, 35000);
    QVERIFY(!core.keepAlive(token));
    QVERIFY(core.start(configuration(), true) > token);
}

void TestDomainRoutingOwnership::cleanup()
{
    QVERIFY(Xray::getInstance().stopAny());
}

QTEST_GUILESS_MAIN(TestDomainRoutingOwnership)
#include "testDomainRoutingOwnership.moc"
