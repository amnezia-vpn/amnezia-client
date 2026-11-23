#include <QQmlProperty>
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

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

    QQuickItem *findMoreButton() const
    {
        const QList<QQuickItem *> buttons = m_fixture.itemsOfType(QStringLiteral("IconButtonType"));

        for (QQuickItem *button : buttons) {
            if (!button->isVisible() || !button->isEnabled()) {
                continue;
            }

            const QUrl source = QQmlProperty(button, QStringLiteral("icon.source")).read().toUrl();

            if (source.toString().contains(QStringLiteral("more-vertical"))) {
                return button;
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
        QVERIFY(m_fixture.pageController()->isStartPageVisible());

        QQuickItem *moreButton = findMoreButton();

        QVERIFY2(moreButton, "Visible More button was not found");

        m_fixture.click(moreButton);

        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Settings")) != nullptr);
        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Enable logs")) != nullptr);
    }

    void loggingSwitchCanBeToggledFromMoreMenu()
    {
        QVERIFY(m_fixture.loadPageFromStartPage(PageLoader::PageEnum::PageSetupWizardConfigSource));
        QVERIFY(m_fixture.pageController()->isStartPageVisible());

        QQuickItem *moreButton = findMoreButton();

        QVERIFY2(moreButton, "Visible More button was not found");

        m_fixture.click(moreButton);

        QQuickItem *loggingRow = nullptr;

        QTRY_VERIFY((loggingRow = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Enable logs"))) != nullptr);

        const QList<QQuickItem *> switches = m_fixture.itemsOfType(QStringLiteral("SwitchType"));

        QVERIFY2(!switches.isEmpty(), "Logging switch was not found");

        QQuickItem *loggingSwitch = nullptr;

        for (QQuickItem *candidate : switches) {
            if (candidate->isVisible() && candidate->property("checked").isValid()) {
                loggingSwitch = candidate;
                break;
            }
        }

        QVERIFY2(loggingSwitch, "Visible logging switch was not found");

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
