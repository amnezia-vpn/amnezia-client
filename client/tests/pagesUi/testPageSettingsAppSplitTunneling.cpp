#include <QQuickItem>
#include <QTest>

#include "pagesUiTestFixture.h"

class PageSettingsAppSplitTunnelingTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    QQuickItem *findSwitch() const
    {
        const QList<QQuickItem *> switches = m_fixture.itemsOfType(QStringLiteral("SwitchType"));

        if (switches.isEmpty()) {
            return nullptr;
        }

        return switches.first();
    }

    QQuickItem *findModeSelector() const
    {
        const QList<QQuickItem *> selectors = m_fixture.itemsOfType(QStringLiteral("SegmentedControlType"));

        if (selectors.isEmpty()) {
            return nullptr;
        }

        return selectors.first();
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

    void pageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAppSplitTunneling")));
        QVERIFY(m_fixture.page());
    }

    void appSplitTunnelingControlsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAppSplitTunneling")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("App split tunneling")));
        QVERIFY(findSwitch());

        QQuickItem *modeSelector = findModeSelector();

        QVERIFY(modeSelector);
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Via VPN")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Directly")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("placeholderText"), QStringLiteral("application name")));
    }

    void splitTunnelingCanBeEnabled()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAppSplitTunneling")));

        QQuickItem *splitTunnelingSwitch = findSwitch();

        QVERIFY(splitTunnelingSwitch);
        QVERIFY(splitTunnelingSwitch->isEnabled());

        const bool initialState = splitTunnelingSwitch->property("checked").toBool();

        m_fixture.click(splitTunnelingSwitch);

        QTRY_VERIFY(splitTunnelingSwitch->property("checked").toBool() != initialState);

        m_fixture.click(splitTunnelingSwitch);

        QTRY_COMPARE(splitTunnelingSwitch->property("checked").toBool(), initialState);
    }

    void routeModeIsDisplayed()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAppSplitTunneling")));

        QQuickItem *modeSelector = findModeSelector();

        QVERIFY(modeSelector);

        const int currentIndex = modeSelector->property("currentIndex").toInt();

        QVERIFY(currentIndex == 0 || currentIndex == 1);
    }

#ifdef Q_OS_WIN

    void routeModeSelectorIsDisabledOnWindows()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAppSplitTunneling")));

        QQuickItem *modeSelector = findModeSelector();

        QVERIFY(modeSelector);
        QCOMPARE(modeSelector->property("enabled").toBool(), false);
    }

    void windowsModeNotificationIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAppSplitTunneling")));

        QQuickItem *notification = m_fixture.visibleItem(
                QStringLiteral("text"),
                QStringLiteral("Only \"Apps from the list should not have access via VPN\" mode is available on Windows"));

        QVERIFY2(notification, "Windows split tunneling notification was not found");
    }

#endif

    void applicationListIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAppSplitTunneling")));

        QVERIFY(m_fixture.page());

        const QList<QQuickItem *> listViews = m_fixture.itemsOfType(QStringLiteral("ListViewType"));

        QVERIFY2(!listViews.isEmpty(), "Application ListView was not found");
    }

    void addApplicationFieldIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAppSplitTunneling")));

        QQuickItem *applicationField = m_fixture.visibleItem(QStringLiteral("placeholderText"), QStringLiteral("application name"));

        QVERIFY(applicationField);
        QVERIFY(applicationField->isEnabled());
    }
};

int runPageSettingsAppSplitTunnelingTests(int argc, char *argv[])
{
    PageSettingsAppSplitTunnelingTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsAppSplitTunneling.moc"
