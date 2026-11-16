#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "ui/controllers/qml/pageController.h"
#include "ui/models/newsModel.h"
#include "ui/utils/pageEnum.h"

#include "pagesUiTestFixture.h"

class TestPageSettingsNewsNotifications : public QObject
{
    Q_OBJECT

private:
    PagesUiTestFixture m_fixture;

    NewsModel *newsModel() const
    {
        if (!m_fixture.engine()) {
            return nullptr;
        }

        QObject *object = m_fixture.engine()->rootContext()->contextProperty(QStringLiteral("NewsModel")).value<QObject *>();
        return qobject_cast<NewsModel *>(object);
    }

    QJsonObject makeNews(const QString &id, const QString &title, const QString &content, const QString &timestamp)
    {
        return { { QStringLiteral("id"), id },
                 { QStringLiteral("title"), title },
                 { QStringLiteral("content"), content },
                 { QStringLiteral("timestamp"), timestamp } };
    }

    void prepareNews()
    {
        NewsModel *model = newsModel();
        Q_ASSERT(model);

        const QString prefix = QUuid::createUuid().toString(QUuid::WithoutBraces);

        QJsonArray news { makeNews(prefix + QStringLiteral("-1"), QStringLiteral("First test news"),
                                   QStringLiteral("First test news content"), QStringLiteral("2026-11-14T10:00:00Z")),

                          makeNews(prefix + QStringLiteral("-2"), QStringLiteral("Second test news"),
                                   QStringLiteral("Second test news content"), QStringLiteral("2026-11-15T10:00:00Z")),

                          makeNews(prefix + QStringLiteral("-3"), QStringLiteral("Third test news"),
                                   QStringLiteral("Third test news content"), QStringLiteral("2026-11-16T10:00:00Z")) };

        model->setNewsList(news);
    }

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

        if (!QTest::qWaitFor([&navigationSpy] { return !navigationSpy.isEmpty(); }, 5000)) {
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
        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsNewsNotifications")));
        QVERIFY(m_fixture.page());
    }

    void newsListIsVisible()
    {
        prepareNews();

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsNewsNotifications")));

        NewsModel *model = newsModel();

        QVERIFY(model);
        QCOMPARE(model->rowCount(), 3);
        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("First test news")));
        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Second test news")));
        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Third test news")));
    }

    void newsItemsAreSortedByTimestamp()
    {
        prepareNews();

        NewsModel *model = newsModel();

        QVERIFY(model);
        QCOMPARE(model->data(model->index(0, 0), NewsModel::TitleRole).toString(), QStringLiteral("Third test news"));
        QCOMPARE(model->data(model->index(1, 0), NewsModel::TitleRole).toString(), QStringLiteral("Second test news"));
        QCOMPARE(model->data(model->index(2, 0), NewsModel::TitleRole).toString(), QStringLiteral("First test news"));
    }

    void clickingUnreadNewsMarksItAsRead()
    {
        prepareNews();

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsNewsNotifications")));

        NewsModel *model = newsModel();

        QVERIFY(model);

        const int newsIndex = 0;

        QCOMPARE(model->data(model->index(newsIndex, 0), NewsModel::IsReadRole).toBool(), false);

        QQuickItem *newsItem = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Third test news"));

        QVERIFY2(newsItem, "News item was not found");

        m_fixture.click(newsItem);

        QTRY_VERIFY(model->data(model->index(newsIndex, 0), NewsModel::IsReadRole).toBool());
        QVERIFY(!model->hasUnread());
    }

    void clickingNewsUpdatesProcessedIndex()
    {
        prepareNews();

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsNewsNotifications")));

        NewsModel *model = newsModel();

        QVERIFY(model);

        QQuickItem *newsItem = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Third test news"));

        QVERIFY(newsItem);

        m_fixture.click(newsItem);

        QTRY_COMPARE(model->processedIndex(), 0);
        QCOMPARE(model->data(model->index(0, 0), NewsModel::IsProcessedRole).toBool(), true);
    }

    void clickingNewsOpensDetailsPage()
    {
        prepareNews();

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsNewsNotifications")));

        QQuickItem *newsItem = m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Third test news"));

        QVERIFY(newsItem);
        QVERIFY(verifyNavigation(newsItem, PageLoader::PageEnum::PageSettingsNewsDetail));
    }

    void newsDetailsContainSelectedNews()
    {
        prepareNews();

        NewsModel *model = newsModel();

        QVERIFY(model);

        model->setProcessedIndex(0);

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsNewsDetail")));
        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Third test news")));
        QTRY_VERIFY(m_fixture.visibleItem(QStringLiteral("text"), QStringLiteral("Third test news content")));
    }

    void emptyNewsListIsHandled()
    {
        NewsModel *model = newsModel();

        QVERIFY(model);

        model->setNewsList(QJsonArray());

        QVERIFY(m_fixture.loadPage(QStringLiteral("PageSettingsNewsNotifications")));
        QCOMPARE(model->rowCount(), 0);
        QVERIFY(m_fixture.page());
    }
};

int runPageSettingsNewsNotificationsTests(int argc, char *argv[])
{
    TestPageSettingsNewsNotifications test;
    return QTest::qExec(&test, argc, argv);
}

#include "testPageSettingsNewsNotifications.moc"
