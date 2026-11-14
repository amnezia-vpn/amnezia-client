#include <QSignalSpy>
#include <QTest>

#include "ui/controllers/qml/pageController.h"
#include "ui/utils/pageEnum.h"

#include "pagesUiTestFixture.h"

class TestPageSettingsApplication : public QObject
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

    QQuickItem *switchForRow(QQuickItem *row)
    {
        if (!row) {
            return nullptr;
        }

        const QList<QQuickItem *> switches = m_fixture.itemsOfType(QStringLiteral("SwitchType"), row);

        for (QQuickItem *candidate : switches) {
            if (candidate->property("checked").isValid()) {
                return candidate;
            }
        }

        return nullptr;
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
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsApplication")));
        QVERIFY(m_fixture.page());
    }

    void applicationSettingsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsApplication")));

        const QStringList expectedOptions { QStringLiteral("Application"),
                                            QStringLiteral("Auto start"),
                                            QStringLiteral("Auto connect"),
                                            QStringLiteral("Start minimized"),
                                            QStringLiteral("Check for updates automatically"),
                                            QStringLiteral("Language"),
                                            QStringLiteral("Logging"),
                                            QStringLiteral("Reset settings and remove all data from the application") };

        for (const QString &option : expectedOptions) {
            QQuickItem *item = m_fixture.visibleItem(QStringLiteral("text"), option);

            QVERIFY2(item, qPrintable(QStringLiteral("Application setting not found: %1").arg(option)));
        }
    }

    void autoConnectCanBeToggled()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsApplication")));

        QQuickItem *row = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Auto connect"));

        QVERIFY2(row, "Auto connect setting was not found");

        QQuickItem *toggle = switchForRow(row);

        QVERIFY2(toggle, "Auto connect switch was not found");

        const bool initialState = toggle->property("checked").toBool();

        m_fixture.click(row);

        QTRY_VERIFY(toggle->property("checked").toBool() != initialState);

        m_fixture.click(row);

        QTRY_COMPARE(toggle->property("checked").toBool(), initialState);
    }

    void autoUpdateCheckCanBeToggled()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsApplication")));

        QQuickItem *row = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Check for updates automatically"));

        QVERIFY2(row, "Automatic update check setting was not found");

        QQuickItem *toggle = switchForRow(row);

        QVERIFY2(toggle, "Automatic update check switch was not found");

        const bool initialState = toggle->property("checked").toBool();

        m_fixture.click(row);

        QTRY_VERIFY(toggle->property("checked").toBool() != initialState);

        m_fixture.click(row);

        QTRY_COMPARE(toggle->property("checked").toBool(), initialState);
    }

    void languageSettingIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsApplication")));

        QQuickItem *languageRow = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Language"));

        QVERIFY2(languageRow, "Language setting was not found");
        QVERIFY(languageRow->isEnabled());
    }

    void opensLoggingSettings()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsApplication")));

        QQuickItem *loggingRow = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Logging"));

        QVERIFY2(loggingRow, "Logging setting was not found");
        QVERIFY(verifyNavigation(loggingRow, PageLoader::PageEnum::PageSettingsLogging));
    }
};

int runPageSettingsApplicationTests(int argc, char *argv[])
{
    TestPageSettingsApplication test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsApplication.moc"
