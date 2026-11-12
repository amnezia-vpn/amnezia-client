#include <QtTest>

#include "pagesUiTestFixture.h"

class PageSettingsKillSwitchTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

private slots:
    void initTestCase()
    {
        QVERIFY(m_fixture.initialize(this));
    }

    void cleanupTestCase()
    {
        m_fixture.cleanup();
    }

    void pageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(m_fixture.page() != nullptr);
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("KillSwitch")) != nullptr);
    }

    void killSwitchDescriptionIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"),
                                      QStringLiteral("Enable to ensure network traffic goes through a secure VPN tunnel, preventing accidental "
                                                     "exposure of your IP and DNS queries if the connection drops")) != nullptr);
    }

    void killSwitchSwitchIsPresent()
    {
        QVERIFY(m_fixture.initialize(this));
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitch")));

        const auto switches = m_fixture.itemsOfType("SwitchType");

        QCOMPARE(switches.size(), 1);
    }

    void softKillSwitchOptionIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Soft KillSwitch")) != nullptr);
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Internet access is blocked if the VPN disconnects unexpectedly")) != nullptr);
    }

    void strictKillSwitchOptionIsHidden()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(!m_fixture.visibleItem("text", "Strict KillSwitch"));
    }

    void dnsExceptionsEntryIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("DNS Exceptions")) != nullptr);
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("DNS servers listed here will remain accessible when KillSwitch is active.")) != nullptr);
    }
};

int runPageSettingsKillSwitchTests(int argc, char *argv[])
{
    PageSettingsKillSwitchTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsKillSwitch.moc"
