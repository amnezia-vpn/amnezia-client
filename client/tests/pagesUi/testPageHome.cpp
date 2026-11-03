#include <QMetaObject>
#include <QObject>
#include <QTest>
#include <QVariant>

#include "pagesUiTestFixture.h"

class PageHomeTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

private slots:
    void initTestCase()
    {
        QVERIFY(m_fixture.initialize(this));
    }

    void cleanup()
    {
        m_fixture.closePage();
    }

    void cleanupTestCase()
    {
        m_fixture.cleanup();
    }

    void pageHomeLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageHome")));
        QVERIFY(m_fixture.page());
    }

    void apiProtocolStateIsInitiallyEmpty()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageHome")));
        QCOMPARE(m_fixture.page()->property("apiAvailableProtocols").toList(), QVariantList { });
        QCOMPARE(m_fixture.page()->property("apiCurrentProtocol").toString(), QString());
    }

    void protocolDisplayName_data()
    {
        QTest::addColumn<QString>("protocol");
        QTest::addColumn<QString>("expectedName");

        QTest::newRow("awg") << QStringLiteral("awg") << QStringLiteral("AmneziaWG");
        QTest::newRow("vless") << QStringLiteral("vless") << QStringLiteral("VLESS");
        QTest::newRow("wireguard") << QStringLiteral("wireguard") << QStringLiteral("wireguard");
        QTest::newRow("empty") << QString() << QString();
    }

    void protocolDisplayName()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageHome")));

        QFETCH(QString, protocol);
        QFETCH(QString, expectedName);

        QVariant result;

        const bool invoked =
                QMetaObject::invokeMethod(m_fixture.page(), "protocolDisplayName", Q_RETURN_ARG(QVariant, result),
                                          Q_ARG(QVariant, QVariant(protocol)));

        QVERIFY2(invoked, "Could not invoke PageHome.protocolDisplayName()");
        QCOMPARE(result.toString(), expectedName);
    }
};

int runPageHomeTests(int argc, char *argv[])
{
    PageHomeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageHome.moc"
