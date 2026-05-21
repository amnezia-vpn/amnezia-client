#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "core/controllers/coreController.h"
#include "core/models/serverDescription.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestUiNewsModelAndController : public QObject
{
    Q_OBJECT

private:
    CoreController *m_coreController;
    SecureQSettings *m_settings;

    // TODO: add env vars for api

private slots:
    void initTestCase()
    {
        QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new CoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init()
    {
        m_settings->clearSettings();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), -1);
        }
    }

    void testRolesAndSignals()
    {
        QSignalSpy fetchNewsFinishedSpy(m_coreController->m_apiNewsUiController, &ApiNewsUiController::fetchNewsFinished);
        QSignalSpy errorOccurredSpy(m_coreController->m_apiNewsUiController, &ApiNewsUiController::errorOccurred);
        QSignalSpy processedIndexChangedSpy(m_coreController->m_newsModel, &NewsModel::processedIndexChanged);
        QSignalSpy hasUnreadChangedSpy(m_coreController->m_newsModel, &NewsModel::hasUnreadChanged);

        /* TODO:
        m_coreController->m_apiNewsUiController->fetchNews(false);
        QVERIFY(errorOccurredSpy.count() == 0, "errorOccurred signal should not be emitted");
        QVERIFY(fetchNewsFinishedSpy.count() == 1, "fetchNewsFinished signal should be emitted");

        m_coreController->m_newsModel->updateModel();
        QVERIFY(hasUnreadChangedSpy.count() == 1, "hasUnreadChanged signal should be emitted");

        QModelIndex newsModelIndex = m_coreController->m_newsModel->index(0, 0);
        QVERIFY2(newsModelIndex.isValid(), "News model index should be valid");

        auto newsId = m_coreController->m_newsModel->data(newsModelIndex, NewsModel::IdRole);
        QCOMPARE(newsId, );

        auto newsTitle = m_coreController->m_newsModel->data(newsModelIndex, NewsModel::TitleRole);
        QCOMPARE(newsTitle, );

        auto newsContent = m_coreController->m_newsModel->data(newsModelIndex, NewsModel::ContentRole);
        QCOMPARE(newsContent, );

        auto newsTimestamp = m_coreController->m_newsModel->data(newsModelIndex, NewsModel::TimestampRole);
        QCOMPARE(newsTimestamp, );

        auto newsIsRead = m_coreController->m_newsModel->data(newsModelIndex, NewsModel::IsReadRole);
        QCOMPARE(newsIsRead, false);

        auto newsIsProcessed = m_coreController->m_newsModel->data(newsModelIndex, NewsModel::IsProcessedRole);
        QCOMPARE(newsIsProcessed, );

        m_coreController->m_newsModel->markAsRead(0);
        ? m_coreController->m_newsModel->updateModel(); ?
        QVERIFY(hasUnreadChangedSpy.count() == 2, "hasUnreadChanged signal should be emitted");
        QCOMPARE(newsIsRead, true);
        */
    }
};

QTEST_MAIN(TestUiNewsModelAndController)
#include "testUiNewsModelAndController.moc"
