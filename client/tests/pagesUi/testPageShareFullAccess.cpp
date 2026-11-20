#include <QQuickItem>
#include <QTest>

#include "pagesUiTestFixture.h"

class TestPageShareFullAccess : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    bool containsVisibleText(const QString &text) const
    {
        for (QQuickItem *item : m_fixture.items()) {
            const QString itemText = item->property("text").toString();

            if (item->isVisible() && itemText.contains(text)) {
                return true;
            }
        }

        return false;
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
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShareFullAccess")));
        QVERIFY(m_fixture.page());
    }

    void pageTitleIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShareFullAccess")));

        QQuickItem *title = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Full access to the server and VPN"));

        QVERIFY2(title, "Page title was not found");
    }

    void fullAccessWarningIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShareFullAccess")));
        QVERIFY(containsVisibleText(QStringLiteral("We recommend that you use full access")));
        QVERIFY(containsVisibleText(QStringLiteral("they can remove and add protocols and services")));
    }

    void serverSelectorIsVisible()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShareFullAccess")));

        QQuickItem *serverSelector = m_fixture.visibleItem(QStringLiteral("objectName"), QStringLiteral("serverSelector"));

        QVERIFY2(serverSelector, "Server selector was not found");
        QVERIFY(serverSelector->isEnabled());
    }

    void shareButtonIsVisibleAndEnabled()
    {
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageShareFullAccess")));

        QQuickItem *shareButton = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Share"));

        QVERIFY2(shareButton, "Share button was not found");
        QVERIFY(shareButton->isEnabled());
    }
};

int runPageShareFullAccessTests(int argc, char *argv[])
{
    TestPageShareFullAccess test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageShareFullAccess.moc"
