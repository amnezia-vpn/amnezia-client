#include <QSignalSpy>
#include <QTest>

#include "pagesUiTestFixture.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/utils/pageEnum.h"

class TestPageSettingsConnection : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    bool verifyNavigation(QQuickItem *target, PageLoader::PageEnum expectedPage)
    {
        if (!target) {
            return false;
        }

        QSignalSpy navigationSpy(m_fixture.pageController(), &PageController::goToPage);

        if (!navigationSpy.isValid()) {
            return false;
        }

        m_fixture.click(target);

        if (!QTest::qWaitFor([&navigationSpy] { return navigationSpy.count() == 1; }, 5000)) {
            return false;
        }

        const QList<QVariant> arguments = navigationSpy.takeFirst();
        return !arguments.isEmpty() && arguments.at(0).toInt() == static_cast<int>(expectedPage);
    }

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

    // Connection settings page

    void pageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsConnection")));
        QVERIFY(m_fixture.page());
    }

    void connectionOptionsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsConnection")));

        const QStringList expectedOptions { QStringLiteral("Use AmneziaDNS"), QStringLiteral("DNS servers"),
                                            QStringLiteral("Site-based split tunneling") };

        for (const QString &option : expectedOptions) {
            QQuickItem *item = m_fixture.visibleItem(QStringLiteral("text"), option);

            QVERIFY2(item, qPrintable(QStringLiteral("Option not found: %1").arg(option)));
        }
    }

    void amneziaDnsSwitchCanBeToggled()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsConnection")));

        QQuickItem *dnsRow = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Use AmneziaDNS"));

        QVERIFY(dnsRow);

        QQuickItem *dnsSwitch = nullptr;

        for (QQuickItem *candidate : m_fixture.itemsOfType(QStringLiteral("SwitchType"))) {

            if (candidate->isVisible() && candidate->property("checked").isValid()) {
                dnsSwitch = candidate;
                break;
            }
        }

        QVERIFY2(dnsSwitch, "AmneziaDNS switch was not found");

        const bool initialState = dnsSwitch->property("checked").toBool();

        m_fixture.click(dnsRow);

        QTRY_VERIFY(dnsSwitch->property("checked").toBool() != initialState);

        m_fixture.click(dnsRow);

        QTRY_COMPARE(dnsSwitch->property("checked").toBool(), initialState);
    }

    void opensDnsSettings()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsConnection")));

        QQuickItem *dnsButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("DNS servers"));

        QVERIFY(dnsButton);
        QVERIFY(verifyNavigation(dnsButton, PageLoader::PageEnum::PageSettingsDns));
    }

    void opensSiteSplitTunnelingSettings()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsConnection")));

        QQuickItem *splitTunnelingButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Site-based split tunneling"));

        QVERIFY(splitTunnelingButton);
        QVERIFY(verifyNavigation(splitTunnelingButton, PageLoader::PageEnum::PageSettingsSplitTunneling));
    }

    void opensAppSplitTunnelingSettings()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsConnection")));

        QQuickItem *appSplitTunnelingButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("App-based split tunneling"));

        QVERIFY(appSplitTunnelingButton);
        QVERIFY(verifyNavigation(appSplitTunnelingButton, PageLoader::PageEnum::PageSettingsAppSplitTunneling));
    }

    void opensKillSwitchSettings()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsConnection")));

        QQuickItem *killSwitchButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("KillSwitch"));

        if (!killSwitchButton) {
            QSKIP("KillSwitch option is not available on this platform");
        }

        QVERIFY(verifyNavigation(killSwitchButton, PageLoader::PageEnum::PageSettingsKillSwitch));
    }

    void dnsSettingsPageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsDns")));
        QVERIFY(m_fixture.page());

        QQuickItem *primaryDns = m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Primary DNS"));
        QQuickItem *secondaryDns = m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Secondary DNS"));

        QVERIFY2(primaryDns, "Primary DNS field was not found");
        QVERIFY2(secondaryDns, "Secondary DNS field was not found");
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Save")));
    }

    void killSwitchPageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitch")));
        QVERIFY(m_fixture.page());
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("KillSwitch")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Soft KillSwitch")));
    }
};

int runPageSettingsConnectionTests(int argc, char *argv[])
{
    TestPageSettingsConnection test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsConnection.moc"
