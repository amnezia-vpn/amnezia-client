#include <QTest>

#include "pagesUiTestFixture.h"

class PageShareTest : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

private slots:
    void init()
    {
        QVERIFY(m_fixture.initialize());
    }

    void cleanup()
    {
        m_fixture.cleanup();
    }

    void pageShareLoads()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Share VPN Access")));
        QVERIFY(m_fixture.clickable(QStringLiteral("Connection")));
        QVERIFY(m_fixture.clickable(QStringLiteral("Users")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("User name")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Server")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Protocol")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Connection format")));
        QVERIFY(m_fixture.clickable(QStringLiteral("Share")));
    }

    void switchesBetweenConnectionAndUsers()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));

        // Switch to Users tab
        QQuickItem *usersButton = m_fixture.clickable(QStringLiteral("Users"));

        QVERIFY(usersButton);
        m_fixture.click(usersButton);

        // Switch back to Connection tab
        QQuickItem *connectionButton = m_fixture.clickable(QStringLiteral("Connection"));

        QVERIFY(connectionButton);
        m_fixture.click(connectionButton);

        // Connection controls should be available again
        QVERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("User name")));
        QVERIFY(m_fixture.clickable(QStringLiteral("Share")));
    }
};


void navigatesToPageShareFullAccess()
{
    QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));

    QQuickItem *moreButton = nullptr;

    const auto iconButtons = m_fixture.itemsOfType(QStringLiteral("IconButtonType"));

    for (QQuickItem *button : iconButtons) {
        if (button->isVisible() && button->isEnabled()) {
            moreButton = button;
            break;
        }
    }

    QVERIFY2(moreButton, "More button was not found");

    m_fixture.click(moreButton);

    QQuickItem *fullAccessButton = nullptr;

    const auto listItems = m_fixture.itemsOfType(QStringLiteral("ListItemType"));

    for (QQuickItem *item : listItems) {
        if (item->isVisible() && item->isEnabled() && item->property("text").toString() == QStringLiteral("Share")) {
            fullAccessButton = item;
            break;
        }
    }

    QVERIFY2(fullAccessButton, "Full access Share button was not found");
    QVERIFY(m_fixture.verifyNavigation(fullAccessButton, PageLoader::PageEnum::PageShareFullAccess));
}


int runPageShareTests(int argc, char *argv[])
{
    PageShareTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageShare.moc"
