#include <QQuickItem>
#include <QTest>

#include "pagesUiTestFixture.h"
#include "ui/utils/pageEnum.h"

class TestPageShare : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    QQuickItem *segment(const QString &text) const
    {
        for (QQuickItem *item : m_fixture.itemsOfType(QStringLiteral("SegmentType"))) {
            if (item->isVisible() && item->property("text").toString() == text) {
                return item;
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
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));
        QVERIFY(m_fixture.page());
    }

    void connectionFieldsAreVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("User name")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Server")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Protocol")));
        QVERIFY(m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Connection format")));
    }

    void switchesBetweenConnectionAndUsers()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));

        QQuickItem *connectionSegment = segment(QStringLiteral("Connection"));
        QQuickItem *usersSegment = segment(QStringLiteral("Users"));

        QVERIFY(connectionSegment);
        QVERIFY(usersSegment);

        QCOMPARE(connectionSegment->property("checked").toBool(), true);
        QCOMPARE(usersSegment->property("checked").toBool(), false);

        m_fixture.click(usersSegment);

        QTRY_VERIFY(usersSegment->property("checked").toBool());
        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Users")));
        QVERIFY(!m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("User name")));

        m_fixture.click(connectionSegment);

        QTRY_VERIFY(connectionSegment->property("checked").toBool());
        QVERIFY(m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("User name")));
    }

    void connectionFieldsAreHiddenOnUsersTab()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));

        QQuickItem *usersSegment = segment(QStringLiteral("Users"));
        QVERIFY(usersSegment);

        m_fixture.click(usersSegment);

        QTRY_VERIFY(usersSegment->property("checked").toBool());
        QVERIFY(!m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("User name")));
        QVERIFY(!m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Connection format")));
    }

    void opensUsersSearch()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));

        QQuickItem *usersSegment = segment(QStringLiteral("Users"));
        QVERIFY(usersSegment);

        m_fixture.click(usersSegment);
        QTRY_VERIFY(usersSegment->property("checked").toBool());

        // The search button belongs to the Users section header.
        QQuickItem *usersHeader = m_fixture.visibleItem(QStringLiteral("headerText"), QStringLiteral("Users"));
        QVERIFY(usersHeader);

        QQuickItem *searchButton = nullptr;

        for (QQuickItem *candidate : m_fixture.itemsOfType(QStringLiteral("IconButtonType"))) {
            if (candidate->isVisible()) {
                searchButton = candidate;
                break;
            }
        }

        QVERIFY2(searchButton, "Users search button was not found");

        m_fixture.click(searchButton);

        QTRY_VERIFY(m_fixture.page()->property("isSearchBarVisible").toBool());
    }

    void navigatesToPageShareFullAccess()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShare")));

        QQuickItem *moreButton = nullptr;

        for (QQuickItem *candidate : m_fixture.itemsOfType(QStringLiteral("IconButtonType"))) {
            if (candidate->isVisible() && candidate->isEnabled()) {
                moreButton = candidate;
                break;
            }
        }

        QVERIFY2(moreButton, "More actions button was not found");

        m_fixture.click(moreButton);

        QQuickItem *shareFullAccessButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Share"));

        QVERIFY2(shareFullAccessButton, "Share full access menu item was not found");
        QVERIFY(m_fixture.verifyNavigation(shareFullAccessButton, PageLoader::PageEnum::PageShareFullAccess));
    }
};

int runPageShareTests(int argc, char *argv[])
{
    TestPageShare test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageShare.moc"
