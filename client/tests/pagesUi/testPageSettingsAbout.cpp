#include <QTest>
#include <QQuickItem>

#include "pagesUiTestFixture.h"

class TestPageSettingsAbout : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

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
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAbout")));
        QVERIFY(m_fixture.page());
    }

    void supportInformationIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAbout")));

        QQuickItem *title = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Support Amnezia"));

        QVERIFY2(title, "Support Amnezia title was not found");

        QQuickItem *description = m_fixture.visibleItem(
            QStringLiteral("text"),
            QStringLiteral("Amnezia is a free and open-source application. "
                           "You can support the developers if you like it."));

        QVERIFY2(description, "Support description was not found");
    }

    void contactsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAbout")));

        const QStringList contacts = {
            QStringLiteral("Telegram group"),
            QStringLiteral("Amnezia Hub Telegram Bot"),
            QStringLiteral("support@amnezia.org"),
            QStringLiteral("GitHub"),
            QStringLiteral("Website")
        };

        for (const QString &contact : contacts) {
            QQuickItem *item = m_fixture.visibleItem(QStringLiteral("text"), contact);

            QVERIFY2(item, qPrintable(QStringLiteral("Contact was not found: %1").arg(contact)));
        }
    }

    void contactDescriptionsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAbout")));

        const QStringList descriptions = {
            QStringLiteral("To discuss features"),
            QStringLiteral("Ask us anything"),
            QStringLiteral("For reviews and bug reports"),
            QStringLiteral("Discover the source code"),
            QStringLiteral("Visit official website")
        };

        for (const QString &description : descriptions) {
            QQuickItem *item = m_fixture.visibleItem(QStringLiteral("subtitle"), description);

            QVERIFY2(item, qPrintable(QStringLiteral("Contact description was not found: %1").arg(description)));
        }
    }

    void updateAndPrivacyButtonsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsAbout")));

        QQuickItem *checkUpdatesButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Check for updates"));

        QVERIFY2(checkUpdatesButton, "Check for updates button was not found");
        QVERIFY(checkUpdatesButton->isEnabled());

        QQuickItem *privacyPolicyButton = m_fixture.visibleItem( QStringLiteral("text"), QStringLiteral("Privacy Policy"));

        QVERIFY2(privacyPolicyButton, "Privacy Policy button was not found");
        QVERIFY(privacyPolicyButton->isEnabled());
    }
};

int runPageSettingsAboutTests(int argc, char *argv[])
{
    TestPageSettingsAbout test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsAbout.moc"
