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
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));
        QVERIFY(visibleItem("text", "DNS servers listed here will remain accessible when KillSwitch is active"));

        closePage();
        cleanup();
    }

    void addressListIsPresent()
    {
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));

        const auto listViews = itemsOfType("ListViewType");
        QCOMPARE(listViews.size(), 1);

        closePage();
        cleanup();
    }

    void addressInputIsPresent()
    {
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));

        const auto textFields = itemsOfType("TextFieldType");
        QCOMPARE(textFields.size(), 1);

        closePage();
        cleanup();
    }

    void importExportButtonIsPresent()
    {
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));

        const auto buttons = itemsOfType("IconButtonType");
        QCOMPARE(buttons.size(), 1);

        closePage();
        cleanup();
    }
};

int runPageSettingsKillSwitchExceptionsTests(int argc, char *argv[])
{
    PageSettingsKillSwitchExceptionsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsKillSwitchExceptions.moc"
