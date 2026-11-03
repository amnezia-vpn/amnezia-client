#include <QTest>

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
};

int runPageSetupWizardConfigSourceTests(int argc, char *argv[])
{
    TestPageSetupWizardConfigSource test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSetupWizardConfigSource.moc"
