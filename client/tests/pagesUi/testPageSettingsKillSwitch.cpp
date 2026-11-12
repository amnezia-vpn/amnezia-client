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
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(visibleItem("text",
                            "Enable to ensure network traffic goes through a secure VPN tunnel, preventing accidental "
                            "exposure of your IP and DNS queries if the connection drops"));

        closePage();
        cleanup();
    }

    void killSwitchSwitchIsPresent()
    {
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitch")));

        const auto switches = itemsOfType("SwitchType");

        QCOMPARE(switches.size(), 1);

        closePage();
        cleanup();
    }

    void softKillSwitchOptionIsVisible()
    {
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(visibleItem("text", "Soft KillSwitch"));
        QVERIFY(visibleItem("text", "Internet access is blocked if the VPN disconnects unexpectedly"));

        closePage();
        cleanup();
    }

    void strictKillSwitchOptionIsHidden()
    {
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(!visibleItem("text", "Strict KillSwitch"));

        closePage();
        cleanup();
    }

    void dnsExceptionsEntryIsVisible()
    {
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(visibleItem("text", "DNS Exceptions"));
        QVERIFY(visibleItem("text", "DNS servers listed here will remain accessible when KillSwitch is active."));

        closePage();
        cleanup();
    }
};

int runPageSettingsKillSwitchTests(int argc, char *argv[])
{
    PageSettingsKillSwitchTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsKillSwitch.moc"
