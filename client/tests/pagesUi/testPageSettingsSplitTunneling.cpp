#include <QMap>
#include <QQuickItem>
#include <QTest>

#include "core/controllers/ipSplitTunnelingController.h"
#include "core/utils/routeModes.h"
#include "ui/controllers/ipSplitTunnelingUiController.h"

#include "pagesUiTestFixture.h"

class PageSettingsSplitTunnelingTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    IpSplitTunnelingController *controller() const
    {
        return m_fixture.core() ? m_fixture.core()->m_ipSplitTunnelingController : nullptr;
    }

    IpSplitTunnelingUiController *uiController() const
    {
        return m_fixture.core() ? m_fixture.core()->m_ipSplitTunnelingUiController : nullptr;
    }

    void setRouteMode(amnezia::RouteMode mode)
    {
        uiController()->setRouteMode(static_cast<int>(mode));
    }

private slots:

    void initTestCase()
    {
        QVERIFY(m_fixture.initialize(this));
        QVERIFY(controller());
        QVERIFY(uiController());
    }

    void cleanup()
    {
        m_fixture.closePage();

        if (controller() && uiController()) {
            controller()->removeSites();

            uiController()->toggleSplitTunneling(false);

            setRouteMode(amnezia::RouteMode::VpnOnlyForwardSites);
        }
    }

    void cleanupTestCase()
    {
        m_fixture.cleanup();
    }

    void pageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsSplitTunneling")));
        QVERIFY(m_fixture.page());
    }

    void splitTunnelingControlsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsSplitTunneling")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Split tunneling")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Via VPN")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Directly")));
        QVERIFY(!m_fixture.itemsOfType(QStringLiteral("SwitchType")).isEmpty());

        const QList<QQuickItem *> panels = m_fixture.itemsOfType(QStringLiteral("AddSitePanel"));

        QVERIFY2(!panels.isEmpty(), "AddSitePanel was not found");
        QCOMPARE(panels.first()->property("placeholderText").toString(), QStringLiteral("website or IP"));
    }

    void splitTunnelingCanBeEnabled()
    {
        uiController()->toggleSplitTunneling(false);

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsSplitTunneling")));

        const QList<QQuickItem *> switches = m_fixture.itemsOfType(QStringLiteral("SwitchType"));

        QVERIFY(!switches.isEmpty());

        QQuickItem *splitTunnelingSwitch = switches.first();

        QVERIFY(splitTunnelingSwitch->isEnabled());
        QCOMPARE(splitTunnelingSwitch->property("checked").toBool(), false);

        m_fixture.click(splitTunnelingSwitch);

        QTRY_VERIFY(uiController()->isSplitTunnelingEnabled());
        QTRY_COMPARE(splitTunnelingSwitch->property("checked").toBool(), true);

        m_fixture.click(splitTunnelingSwitch);

        QTRY_VERIFY(!uiController()->isSplitTunnelingEnabled());
        QTRY_COMPARE(splitTunnelingSwitch->property("checked").toBool(), false);
    }

    void selectsViaVpnMode()
    {
        setRouteMode(amnezia::RouteMode::VpnAllExceptSites);

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsSplitTunneling")));

        QQuickItem *viaVpnButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Via VPN"));

        QVERIFY(viaVpnButton);
        QVERIFY(viaVpnButton->isEnabled());

        m_fixture.click(viaVpnButton);

        QTRY_COMPARE(uiController()->getRouteMode(), static_cast<int>(amnezia::RouteMode::VpnOnlyForwardSites));
        QCOMPARE(m_fixture.page()->property("routeModeIndex").toInt(), 0);
    }

    void selectsDirectlyMode()
    {
        setRouteMode(amnezia::RouteMode::VpnOnlyForwardSites);

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsSplitTunneling")));

        QQuickItem *directlyButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Directly"));

        QVERIFY(directlyButton);
        QVERIFY(directlyButton->isEnabled());

        m_fixture.click(directlyButton);

        QTRY_COMPARE(uiController()->getRouteMode(), static_cast<int>(amnezia::RouteMode::VpnAllExceptSites));
        QCOMPARE(m_fixture.page()->property("routeModeIndex").toInt(), 1);
    }

    void siteListDisplaysAddedAddress()
    {
        QMap<QString, QStringList> sites;

        sites.insert(QStringLiteral("example.com"), QStringList { QStringLiteral("203.0.113.10") });

        controller()->addSites(sites, false);
        uiController()->updateModel();

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsSplitTunneling")));

        QQuickItem *siteItem = nullptr;

        QTRY_VERIFY((siteItem = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("example.com"))) != nullptr);
        QCOMPARE(siteItem->property("subtitle").toString(), QStringLiteral("203.0.113.10"));
    }

    void siteListDisplaysAddedIp()
    {
        const QString ipAddress = QStringLiteral("192.0.2.1/32");

        QMap<QString, QStringList> sites;
        sites.insert(ipAddress, QStringList());

        controller()->addSites(sites, false);
        uiController()->updateModel();

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsSplitTunneling")));

        QQuickItem *siteItem = nullptr;

        QTRY_VERIFY((siteItem = m_fixture.visibleItem(QStringLiteral("text"), ipAddress)) != nullptr);
        QCOMPARE(siteItem->property("text").toString(), ipAddress);
    }
};

int runPageSettingsSplitTunnelingTests(int argc, char *argv[])
{
    PageSettingsSplitTunnelingTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsSplitTunneling.moc"
