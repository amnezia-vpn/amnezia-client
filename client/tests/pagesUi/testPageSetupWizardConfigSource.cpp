#include <QTest>
#include <QSignalSpy>

#include "ui/utils/pageEnum.h"

#include "pagesUiTestFixture.h"

class TestPageSetupWizardConfigSource : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    QQuickItem *findCard(const QString &text) const
    {
        return m_fixture.item("text", text);
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
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSetupWizardConfigSource")));
        QVERIFY(m_fixture.page());
    }

    void connectionOptionsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSetupWizardConfigSource")));

        QQuickItem *amneziaVpnCard = findCard(QStringLiteral("VPN by Amnezia"));
        QQuickItem *selfHostedVpnCard = findCard(QStringLiteral("Self-hosted VPN"));

        QVERIFY2(amneziaVpnCard, "VPN by Amnezia card was not found");
        QVERIFY2(selfHostedVpnCard, "Self-hosted VPN card was not found");

        QVERIFY(amneziaVpnCard->isVisible());
        QVERIFY(selfHostedVpnCard->isVisible());

        QVERIFY(amneziaVpnCard->isEnabled());
        QVERIFY(selfHostedVpnCard->isEnabled());
    }

    void restoringBackupDisablesConnectionOptions()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSetupWizardConfigSource")));

        QQuickItem *amneziaVpnCard = findCard(QStringLiteral("VPN by Amnezia"));
        QQuickItem *selfHostedVpnCard = findCard(QStringLiteral("Self-hosted VPN"));

        QVERIFY(amneziaVpnCard);
        QVERIFY(selfHostedVpnCard);

        QVERIFY(m_fixture.page()->setProperty("isRestoringBackup", true));

        QVERIFY(!amneziaVpnCard->isEnabled());
        QVERIFY(!selfHostedVpnCard->isEnabled());
    }

    void finishingBackupRestoreEnablesConnectionOptions()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSetupWizardConfigSource")));

        QQuickItem *amneziaVpnCard = findCard(QStringLiteral("VPN by Amnezia"));
        QQuickItem *selfHostedVpnCard = findCard(QStringLiteral("Self-hosted VPN"));

        QVERIFY(amneziaVpnCard);
        QVERIFY(selfHostedVpnCard);

        QVERIFY(m_fixture.page()->setProperty("isRestoringBackup", true));

        QVERIFY(!amneziaVpnCard->isEnabled());
        QVERIFY(!selfHostedVpnCard->isEnabled());

        QVERIFY(m_fixture.page()->setProperty("isRestoringBackup", false));

        QVERIFY(amneziaVpnCard->isEnabled());
        QVERIFY(selfHostedVpnCard->isEnabled());
    }

    void selectingSelfHostedVpnOpensCredentialsPage()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSetupWizardConfigSource")));

        QQuickItem *selfHostedVpnCard = findCard(QStringLiteral("Self-hosted VPN"));

        QVERIFY(selfHostedVpnCard);
        QVERIFY(selfHostedVpnCard->isVisible());
        QVERIFY(selfHostedVpnCard->isEnabled());

        QSignalSpy navigationSpy(m_fixture.pageController(), &PageController::goToPage);

        QVERIFY(navigationSpy.isValid());

        m_fixture.click(selfHostedVpnCard);

        QTRY_COMPARE(navigationSpy.count(), 1);

        const QList<QVariant> arguments = navigationSpy.takeFirst();

        QCOMPARE(arguments.at(0).toInt(), static_cast<int>(PageLoader::PageEnum::PageSetupWizardCredentials));
    }

    void moreMenuCanBeOpened()
    {
        QVERIFY(m_fixture.loadPageFromStartPage(PageLoader::PageEnum::PageSetupWizardConfigSource));

        const QList<QQuickItem *> buttons = m_fixture.itemsOfType(QStringLiteral("IconButtonType"));

        QVERIFY2(!buttons.isEmpty(), "More menu button was not found");

        // The first IconButtonType belongs to NavigationBarType
        QQuickItem *moreButton = buttons.first();

        QVERIFY(moreButton->isVisible());
        QVERIFY(moreButton->isEnabled());

        m_fixture.click(moreButton);

        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Settings")) != nullptr);
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Enable logs")) != nullptr);
    }

    void loggingSwitchCanBeToggledFromMoreMenu()
    {
        QVERIFY(m_fixture.loadPageFromStartPage(PageLoader::PageEnum::PageSetupWizardConfigSource));

        const QList<QQuickItem *> buttons = m_fixture.itemsOfType(QStringLiteral("IconButtonType"));

        QVERIFY(!buttons.isEmpty());

        m_fixture.click(buttons.first());

        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Enable logs")) != nullptr);

        QQuickItem *loggingRow = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Enable logs"));

        QVERIFY(loggingRow);

        const QList<QQuickItem *> switches = m_fixture.itemsOfType(QStringLiteral("SwitchType"));

        QVERIFY2(!switches.isEmpty(), "Logging switch was not found");

        QQuickItem *loggingSwitch = nullptr;

        for (QQuickItem *candidate : switches) {
            if (candidate->isVisible() && candidate->property("checked").isValid()) {
                loggingSwitch = candidate;
                break;
            }
        }

        QVERIFY(loggingSwitch);

        const bool initialState = loggingSwitch->property("checked").toBool();

        m_fixture.click(loggingRow);

        QTRY_VERIFY(loggingSwitch->property("checked").toBool() != initialState);

        m_fixture.click(loggingRow);

        QTRY_COMPARE(loggingSwitch->property("checked").toBool(), initialState);
    }
};

int runPageSetupWizardConfigSourceTests(int argc, char *argv[])
{
    TestPageSetupWizardConfigSource test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSetupWizardConfigSource.moc"
