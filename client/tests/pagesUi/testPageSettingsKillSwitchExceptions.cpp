#include <QTest>

#include "pagesUiTestFixture.h"

class PageSettingsKillSwitchExceptionsTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

private slots:
    void init()
    {
        QVERIFY(m_fixture.initialize(this));
    }

    void cleanup()
    {
        m_fixture.cleanup();
    }

    void pageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));
        QVERIFY(m_fixture.page() != nullptr);
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("DNS Exceptions")) != nullptr);
    }

    void descriptionIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("DNS servers listed here will remain accessible when KillSwitch is active")) != nullptr);
    }

    void addressListIsPresent()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));

        const auto listViews = m_fixture.itemsOfType("ListViewType");
        QCOMPARE(listViews.size(), 1);
    }

    void addressInputIsPresent()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));

        const auto textFields = m_fixture.itemsOfType("TextFieldType");
        QCOMPARE(textFields.size(), 1);
    }

    void importExportButtonIsPresent()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));

        const auto buttons = m_fixture.itemsOfType("IconButtonType");
        QCOMPARE(buttons.size(), 2);
    }
};

int runPageSettingsKillSwitchExceptionsTests(int argc, char *argv[])
{
    PageSettingsKillSwitchExceptionsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsKillSwitchExceptions.moc"
