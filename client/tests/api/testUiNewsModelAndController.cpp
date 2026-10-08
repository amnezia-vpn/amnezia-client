#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>
#include <QProcessEnvironment>

#include "utils/testCoreController.h"
#include "utils/testUtils.h""
#include "secureQSettings.h"
#include "vpnConnection.h"

#include "core/models/api/apiV2ServerConfig.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/serverConfigUtils.h"

#include "core/controllers/selfhosted/importController.h"
#include "core/models/serverDescription.h"

#include "ui/controllers/api/apiNewsUiController.h"
#include "ui/models/newsModel.h"

using namespace amnezia;
using namespace amnezia::test;

namespace
{
    QJsonObject gatewayServerConfig()
    {
        QJsonObject lastConfig;
        lastConfig[configKey::hostName] = QStringLiteral("10.0.0.1");
        lastConfig[configKey::clientPrivKey] = QStringLiteral("test-private-key");
        lastConfig[configKey::persistentKeepAlive] = QStringLiteral("25-35");

        QJsonObject awgConfig;
        awgConfig[configKey::lastConfig] = QString(QJsonDocument(lastConfig).toJson(QJsonDocument::Compact));
        awgConfig[configKey::port] = QStringLiteral("35333");

        QJsonObject awgContainer;
        awgContainer[configKey::container] = QStringLiteral("amnezia-awg");
        awgContainer[QString(configKey::awg)] = awgConfig;

        QJsonObject config;
        config[configKey::name] = QStringLiteral("Amnezia Premium");
        config[configKey::description] = QStringLiteral("Premium service");
        config[configKey::hostName] = QStringLiteral("gateway.example.org");
        config[configKey::configVersion] = serverConfigUtils::ConfigSource::AmneziaGateway;
        config[configKey::formatVersion] = serverConfigUtils::currentConfigFormatVersion;
        config[configKey::defaultContainer] = QStringLiteral("amnezia-awg");
        config[configKey::containers] = QJsonArray { awgContainer };

        return config;
    }

    ApiV2ServerConfig gatewayTestSubscription(const QString &apiKey)
    {
        ApiV2ServerConfig config = ApiV2ServerConfig::fromJson(gatewayServerConfig());

        config.apiConfig.serviceType = QStringLiteral("amnezia-premium");
        config.apiConfig.serviceProtocol = QString(configKey::awg);
        config.apiConfig.userCountryCode = QStringLiteral("ru");
        config.apiConfig.vpnKey = QStringLiteral("vpn://stored-key");
        config.authData.apiKey = apiKey;
        config.crc = 42;

        return config;
    }

    QString envValue(const char *name)
    {
        return QProcessEnvironment::systemEnvironment().value(QString::fromLatin1(name)).trimmed();
    }

    QJsonObject makeNewsItem(const QString &id, const QString &title, const QString &content, const QString &timestamp)
    {
        return { { "id", id }, { "title", title }, { "content", content }, { "timestamp", timestamp } };
    }

    QJsonArray makeNews()
    {
        return { makeNewsItem("news-1", "First news", "First content", "2026-09-29T10:00:00Z"),
                 makeNewsItem("news-2", "Second news", "Second content", "2026-09-28T10:00:00Z"),
                 makeNewsItem("news-3", "Third news", "Third content", "2026-09-27T10:00:00Z") };
    }

} // namespace

class TestUiNewsModelAndController : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController;
    SecureQSettings *m_settings;

private slots:
    void initTestCase()
    {
        const QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        const auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
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
        m_coreController->m_serversRepository->invalidateCache();
        if (m_coreController->m_serversModel) {
            m_coreController->m_serversModel->updateModel(QVector<ServerDescription>(), QString { });
        }
        m_coreController->m_newsModel->setNewsList({});
    }

    void testRolesAndSignals()
    {
        NewsModel *model = m_coreController->m_newsModel;
        model->setNewsList(makeNews());

        QSignalSpy hasUnreadChangedSpy(model, &NewsModel::hasUnreadChanged);
        QSignalSpy processedIndexChangedSpy(model, &NewsModel::processedIndexChanged);

        QCOMPARE(model->rowCount(), 3);
        QVERIFY(model->hasUnread());

        const QModelIndex index = model->index(0, 0);

        QVERIFY2(index.isValid(), "News model index should be valid");
        QCOMPARE(model->data(index, NewsModel::IdRole).toString(), QString("news-1"));
        QCOMPARE(model->data(index, NewsModel::TitleRole).toString(), QString("First news"));
        QCOMPARE(model->data(index, NewsModel::ContentRole).toString(), QString("First content"));

        const QDateTime timestamp = QDateTime::fromString(model->data(index, NewsModel::TimestampRole).toString(), Qt::ISODate);

        QVERIFY(timestamp.isValid());

        const QDateTime expectedTimestamp = QDateTime::fromString("2026-09-29T10:00:00Z", Qt::ISODate).toLocalTime();

        QCOMPARE(timestamp, expectedTimestamp);
        QCOMPARE(model->data(index, NewsModel::IsReadRole).toBool(), false);
        QCOMPARE(model->data(index, NewsModel::IsProcessedRole).toBool(), false);

        model->setProcessedIndex(0);

        QCOMPARE(model->processedIndex(), 0);
        QCOMPARE(processedIndexChangedSpy.count(), 1);
        QCOMPARE(model->data(index, NewsModel::IsProcessedRole).toBool(), true);

        model->markAsRead(0);

        QCOMPARE(model->data(index, NewsModel::IsReadRole).toBool(), true);
        QCOMPARE(hasUnreadChangedSpy.count(), 1);
    }

    void testSorting()
    {
        NewsModel *model = m_coreController->m_newsModel;
        model->setNewsList({ makeNewsItem("old", "Old", "Old content", "2026-09-27T10:00:00Z"),
                             makeNewsItem("new", "New", "New content", "2026-09-29T10:00:00Z"),
                             makeNewsItem("middle", "Middle", "Middle content", "2026-09-28T10:00:00Z") });

        QCOMPARE(model->rowCount(), 3);
        QCOMPARE(model->data(model->index(0, 0), NewsModel::IdRole).toString(), QString("new"));
        QCOMPARE(model->data(model->index(1, 0), NewsModel::IdRole).toString(), QString("middle"));
        QCOMPARE(model->data(model->index(2, 0), NewsModel::IdRole).toString(), QString("old"));
    }

    void testInvalidItemsAreIgnored()
    {
        NewsModel *model = m_coreController->m_newsModel;
        model->setNewsList(
                { QJsonValue("invalid"),
                  QJsonObject { { "title", "No id" }, { "content", "Content" }, { "timestamp", "2026-09-29T10:00:00Z" } },
                  makeNewsItem("valid", "Valid", "Valid content", "2026-09-29T10:00:00Z") });

        QCOMPARE(model->rowCount(), 1);
        QCOMPARE(model->data(model->index(0, 0), NewsModel::IdRole).toString(), QString("valid"));
    }

    void testReadStatePersistence()
    {
        m_coreController->m_newsModel->setNewsList(makeNews());
        m_coreController->m_newsModel->markAsRead(0);

        NewsModel restoredModel(m_coreController->m_appSettingsRepository);
        restoredModel.setNewsList(makeNews());

        QCOMPARE(restoredModel.data(restoredModel.index(0, 0), NewsModel::IsReadRole).toBool(), true);
        QCOMPARE(restoredModel.data(restoredModel.index(1, 0), NewsModel::IsReadRole).toBool(), false);
    }

    void testFetchNewsWithoutServers()
    {
        QSignalSpy fetchNewsFinishedSpy(m_coreController->m_apiNewsUiController, &ApiNewsUiController::fetchNewsFinished);
        QSignalSpy errorOccurredSpy(m_coreController->m_apiNewsUiController, &ApiNewsUiController::errorOccurred);

        m_coreController->m_apiNewsUiController->fetchNews(false);

        QTRY_COMPARE_WITH_TIMEOUT(fetchNewsFinishedSpy.count(), 1, 1000);
        QCOMPARE(errorOccurredSpy.count(), 0);
        QCOMPARE(m_coreController->m_newsModel->rowCount(), 0);
    }

    void testFetchNewsFromApi()
    {
        const QString apiKey = envValue("AMNEZIA_TEST_API_KEY");

        if (apiKey.isEmpty()) {
            QSKIP("AMNEZIA_TEST_API_KEY is not set, skipping live gateway test");
        }

        m_settings->setValue("Conf/installationUuid", QUuid::createUuidV5(QUuid(), QByteArrayLiteral("amnezia-news-api-test")).toString(QUuid::WithoutBraces));

        const ApiV2ServerConfig seed = gatewayTestSubscription(apiKey);
        const QString serverId = m_coreController->m_serversRepository->addServer(QString(), seed.toJson(), serverConfigUtils::configTypeFromJson(seed.toJson()));

        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 1);

        SubscriptionController::CaptchaInfo captchaInfo;

        const ErrorCode updateError = m_coreController->m_subscriptionController->updateServiceFromGateway(serverId, QStringLiteral("de"), /*isConnectEvent=*/false, &captchaInfo, nullptr);

        if (updateError == ErrorCode::ApiCaptchaRequiredError) {
            QSKIP("Gateway requested a captcha, skipping live gateway test");
        }

        QCOMPARE(updateError, ErrorCode::NoError);
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 1);

        QSignalSpy fetchNewsFinishedSpy(m_coreController->m_apiNewsUiController, &ApiNewsUiController::fetchNewsFinished);
        QSignalSpy errorOccurredSpy(m_coreController->m_apiNewsUiController, &ApiNewsUiController::errorOccurred);

        m_coreController->m_apiNewsUiController->fetchNews(false);

        QTRY_COMPARE_WITH_TIMEOUT(fetchNewsFinishedSpy.count(), 1, 10000);
        QCOMPARE(errorOccurredSpy.count(), 0);
        QVERIFY(m_coreController->m_newsModel->rowCount() > 0);

        const QModelIndex index = m_coreController->m_newsModel->index(0, 0);

        QVERIFY(index.isValid());
        QVERIFY(!m_coreController->m_newsModel->data(index, NewsModel::IdRole).toString().isEmpty());
        QVERIFY(!m_coreController->m_newsModel->data(index, NewsModel::TitleRole).toString().isEmpty());
        QVERIFY(!m_coreController->m_newsModel->data(index, NewsModel::ContentRole).toString().isEmpty());

        const QDateTime timestamp = QDateTime::fromString(m_coreController->m_newsModel->data(index, NewsModel::TimestampRole).toString(), Qt::ISODate);

        QVERIFY(timestamp.isValid());
        QCOMPARE(m_coreController->m_newsModel->data(index, NewsModel::IsReadRole).toBool(), false);
    }
};

QTEST_MAIN(TestUiNewsModelAndController)
#include "testUiNewsModelAndController.moc"
