#include <QSignalSpy>
#include <QTest>

#include "ui/controllers/qml/pageController.h"
#include "ui/utils/pageEnum.h"

#include "pagesUiTestFixture.h"

class TestPageSettingsLogging : public QObject
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

    void pageLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsLogging")));
        QVERIFY(m_fixture.page());
    }

    void loggingOptionsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsLogging")));

        const QStringList expectedOptions { QStringLiteral("Record logs"),   QStringLiteral("Logs"),
                                            QStringLiteral("Tunnel"),        QStringLiteral("Application"),
                                            QStringLiteral("Save all logs"), QStringLiteral("Clear all logs") };

        for (const QString &option : expectedOptions) {
            QQuickItem *item = m_fixture.visibleItem(QStringLiteral("text"), option);

            QVERIFY2(item, qPrintable(QStringLiteral("Logging option not found: %1").arg(option)));
        }
    }

    void recordLogsSwitchCanBeToggled()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsLogging")));

        QQuickItem *loggingRow = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Record logs"));

        QVERIFY2(loggingRow, "Record logs setting was not found");

        QQuickItem *loggingSwitch = nullptr;

        for (QQuickItem *candidate : m_fixture.itemsOfType(QStringLiteral("SwitchType"))) {
            if (candidate->isVisible() && candidate->property("checked").isValid()) {
                loggingSwitch = candidate;
                break;
            }
        }

        QVERIFY2(loggingSwitch, "Record logs switch was not found");

        const bool initialState = loggingSwitch->property("checked").toBool();

        m_fixture.click(loggingRow);

        QTRY_VERIFY(loggingSwitch->property("checked").toBool() != initialState);

        m_fixture.click(loggingRow);

        QTRY_COMPARE(loggingSwitch->property("checked").toBool(), initialState);
    }

    void opensTunnelLogs()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsLogging")));

        QQuickItem *tunnelRow = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Tunnel"));

        QVERIFY2(tunnelRow, "Tunnel logs option was not found");
        QVERIFY(verifyNavigation(tunnelRow, PageLoader::PageEnum::PageSettingsLogViewer));
    }

    void opensApplicationLogs()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsLogging")));

        QQuickItem *applicationRow = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Application"));

        QVERIFY2(applicationRow, "Application logs option was not found");
        QVERIFY(verifyNavigation(applicationRow, PageLoader::PageEnum::PageSettingsLogViewer));
    }
};

int runPageSettingsLoggingTests(int argc, char *argv[])
{
    TestPageSettingsLogging test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsLogging.moc"
