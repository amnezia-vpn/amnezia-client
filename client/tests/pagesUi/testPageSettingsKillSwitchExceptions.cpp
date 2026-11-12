#include <QtTest>

class PageSettingsKillSwitchExceptionsTest : public PagesUiTestFixture
{
    Q_OBJECT

private slots:
    void pageLoads()
    {
        QVERIFY(initialize(this));
        QVERIFY(loadPage(QStringLiteral("PageSettingsKillSwitchExceptions")));
        QVERIFY(page() != nullptr);
        QVERIFY(visibleItem("text", "DNS Exceptions"));

        closePage();
        cleanup();
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
