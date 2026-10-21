#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QThreadPool>
#include <QUuid>

#include "utils/testCoreController.h"
#include "core/models/api/apiV2ServerConfig.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/serverConfigUtils.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

namespace
{
    const QString favoritesKey = QStringLiteral("Conf/favoriteLocations");
    const QString serverA = QStringLiteral("aaaaaaaa-0000-0000-0000-000000000001");
    const QString serverB = QStringLiteral("bbbbbbbb-0000-0000-0000-000000000002");
    const QString serverSelfHosted = QStringLiteral("cccccccc-0000-0000-0000-000000000003");

    const QStringList locationCodes = { "AT", "BE", "CA", "CH", "CZ", "DE", "DK", "ES", "FI", "FR", "GB",
                                        "HU", "IE", "IT", "JP", "NL", "NO", "PL", "PT", "SE", "US" };

    QJsonArray toJsonArray(const QStringList &codes)
    {
        QJsonArray array;
        for (const QString &code : codes) {
            array.append(code);
        }
        return array;
    }

    QJsonArray countriesJson(const QStringList &codes)
    {
        QJsonArray countries;
        for (const QString &code : codes) {
            QJsonObject country;
            country[apiDefs::key::serverCountryCode] = code;
            country[apiDefs::key::serverCountryName] = QStringLiteral("Country ") + code;
            countries.append(country);
        }
        return countries;
    }

    QJsonObject apiServerJson(const QString &serverId, const QString &hostName)
    {
        ApiV2ServerConfig api;
        api.hostName = hostName;
        api.description = hostName;
        api.configVersion = serverConfigUtils::ConfigSource::AmneziaGateway;
        api.apiConfig.serviceType = QStringLiteral("amnezia-premium");
        api.apiConfig.availableCountries = countriesJson(locationCodes);
        QJsonObject json = api.toJson();
        if (!serverId.isEmpty()) {
            json[configKey::storageServerId] = serverId;
        }
        return json;
    }

    QJsonObject selfHostedServerJson(const QString &serverId, const QString &hostName)
    {
        QJsonObject json;
        json[configKey::hostName] = hostName;
        json[configKey::userName] = QStringLiteral("root");
        json[configKey::password] = QStringLiteral("secret");
        json[configKey::port] = 22;
        json[configKey::description] = hostName;
        json[configKey::storageServerId] = serverId;
        return json;
    }

    QByteArray backupWith(const QString &key, const QJsonValue &value)
    {
        QJsonObject backup;
        backup[key] = value;
        return QJsonDocument(backup).toJson();
    }

    QJsonObject perServer(const QString &serverId, const QStringList &codes)
    {
        QJsonObject favorites;
        favorites[serverId] = toJsonArray(codes);
        return favorites;
    }

    bool hasUseCase(const ApiCountryListModel *model, const QString &id)
    {
        for (const QVariant &entry : model->useCases()) {
            if (entry.toMap().value(QStringLiteral("useCaseId")).toString() == id) {
                return true;
            }
        }
        return false;
    }
} // namespace

class TestFavoriteLocationsRestore : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;

    ApiCountryModel *countryModel() const { return m_coreController->m_apiCountryModel; }
    ApiCountryListModel *picker() const { return m_coreController->m_apiCountryListModel; }
    ApiCountryListModel *nativeConfigs() const { return m_coreController->m_apiConfigsCountryListModel; }
    QStringList storedFavorites(const QString &serverId) const
    {
        return m_coreController->m_appSettingsRepository->favoriteLocations(serverId);
    }

    void storeServers(const QJsonArray &servers)
    {
        m_settings->setValue(QStringLiteral("Servers/serversList"), QJsonDocument(servers).toJson());
        m_coreController->m_serversRepository->invalidateCache();
    }

    void showServer(const QString &serverId, const QStringList &codes = locationCodes)
    {
        countryModel()->setServerId(serverId);
        countryModel()->updateModel(countriesJson(codes), QString(), QString());
    }

    void restore(const QByteArray &data)
    {
        QSignalSpy restoredSpy(m_coreController->m_settingsUiController, &SettingsUiController::restoreBackupFinished);
        m_coreController->m_settingsUiController->restoreAppConfigFromData(data);
        QCOMPARE(restoredSpy.count(), 1);
    }

    void favoriteAll(const QStringList &codes)
    {
        for (const QString &code : codes) {
            QVERIFY(picker()->toggleFavorite(code));
        }
    }

private slots:
    void initTestCase()
    {
        const QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);
        auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);
        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);
    }

    void cleanupTestCase()
    {
        // Restores with API servers start gateway requests that write settings when they finish
        QThreadPool::globalInstance()->waitForDone();
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init()
    {
        // Goes through the repository so that settingsCleared reaches the models
        m_coreController->m_appSettingsRepository->clearSettings();
        m_coreController->m_serversRepository->invalidateCache();
        showServer(serverA);
        QVERIFY(picker()->favorites().isEmpty());
        QVERIFY(nativeConfigs()->favorites().isEmpty());
    }

    void testFavoritesArePerServer()
    {
        favoriteAll({ "US", "DE" });

        showServer(serverB);
        QVERIFY(picker()->favorites().isEmpty());
        QVERIFY(nativeConfigs()->favorites().isEmpty());
        favoriteAll({ "FR" });

        showServer(serverA);
        QCOMPARE(picker()->favorites(), QStringList({ "DE", "US" }));
        QCOMPARE(nativeConfigs()->favorites(), QStringList({ "DE", "US" }));

        QCOMPARE(storedFavorites(serverA), QStringList({ "DE", "US" }));
        QCOMPARE(storedFavorites(serverB), QStringList({ "FR" }));
    }

    void testLimitIsPerServer()
    {
        favoriteAll(locationCodes.mid(0, picker()->favoritesLimit()));
        QVERIFY(!picker()->toggleFavorite(locationCodes.last()));

        showServer(serverB);
        QVERIFY(picker()->toggleFavorite(locationCodes.last()));
    }

    void testToggleWithoutServerIsNotStored()
    {
        showServer(QString());
        QVERIFY(picker()->toggleFavorite("US"));

        QVERIFY(m_settings->value(favoritesKey).toMap().isEmpty());

        showServer(serverA);
        QVERIFY(picker()->favorites().isEmpty());
    }

    void testBackupContainsPerServerFavorites()
    {
        favoriteAll({ "US", "DE", "NL" });
        showServer(serverB);
        favoriteAll({ "FR" });

        const QJsonObject backup = QJsonDocument::fromJson(m_coreController->m_settingsController->backupAppConfig()).object();

        const QVariantMap favorites = backup.value(favoritesKey).toObject().toVariantMap();
        QCOMPARE(favorites.value(serverA).toStringList(), QStringList({ "DE", "NL", "US" }));
        QCOMPARE(favorites.value(serverB).toStringList(), QStringList({ "FR" }));
    }

    void testRestoreReloadsBothModels()
    {
        favoriteAll({ "FR" });

        QSignalSpy pickerChangedSpy(picker(), &ApiCountryListModel::favoritesChanged);
        QSignalSpy nativeChangedSpy(nativeConfigs(), &ApiCountryListModel::favoritesChanged);

        restore(backupWith(favoritesKey, perServer(serverA, { "US", "DE" })));

        const QStringList expected = { "DE", "US" };
        QCOMPARE(storedFavorites(serverA), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);
        QCOMPARE(pickerChangedSpy.count(), 0);
        QCOMPARE(nativeChangedSpy.count(), 0);
    }

    void testRestoreOtherServerOnlyClearsCurrent()
    {
        favoriteAll({ "FR" });

        restore(backupWith(favoritesKey, perServer(serverB, { "US" })));

        QVERIFY(storedFavorites(serverA).isEmpty());
        QVERIFY(picker()->favorites().isEmpty());

        showServer(serverB);
        QCOMPARE(picker()->favorites(), QStringList({ "US" }));
    }

    void testToggleAfterRestoreKeepsRestored()
    {
        favoriteAll({ "FR" });
        restore(backupWith(favoritesKey, perServer(serverA, { "US", "DE" })));

        QVERIFY(picker()->toggleFavorite("NL"));

        const QStringList expected = { "DE", "NL", "US" };
        QCOMPARE(storedFavorites(serverA), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);

        QVERIFY(nativeConfigs()->toggleFavorite("US"));
        QCOMPARE(storedFavorites(serverA), QStringList({ "DE", "NL" }));
        QCOMPARE(picker()->favorites(), QStringList({ "DE", "NL" }));
    }

    void testRestoreWithoutKeyKeepsCurrent()
    {
        favoriteAll({ "FR" });

        restore(backupWith(QStringLiteral("Conf/autoConnect"), false));

        QCOMPARE(storedFavorites(serverA), QStringList({ "FR" }));
        QCOMPARE(picker()->favorites(), QStringList({ "FR" }));
        QCOMPARE(nativeConfigs()->favorites(), QStringList({ "FR" }));
    }

    void testRestoreValueShapes_data()
    {
        QTest::addColumn<QJsonValue>("value");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("empty map") << QJsonValue(QJsonObject()) << QStringList();
        QTest::newRow("null") << QJsonValue(QJsonValue::Null) << QStringList();
        QTest::newRow("number") << QJsonValue(42) << QStringList();
        QTest::newRow("bool") << QJsonValue(true) << QStringList();
        QTest::newRow("server: empty list") << QJsonValue(perServer(serverA, {})) << QStringList();
        QTest::newRow("server: single string") << QJsonValue(QJsonObject { { serverA, "US" } }) << QStringList({ "US" });
        QTest::newRow("server: duplicates and empty")
                << QJsonValue(perServer(serverA, { "US", "US", "", "DE" })) << QStringList({ "DE", "US" });
        QTest::newRow("server: number") << QJsonValue(QJsonObject { { serverA, 42 } }) << QStringList();
        QTest::newRow("server: object") << QJsonValue(QJsonObject { { serverA, QJsonObject() } }) << QStringList();
    }

    void testRestoreValueShapes()
    {
        QFETCH(QJsonValue, value);
        QFETCH(QStringList, expected);

        favoriteAll({ "FR" });

        restore(backupWith(favoritesKey, value));

        QCOMPARE(storedFavorites(serverA), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);
    }

    void testRestoreEmptyResetsFavoritesChip()
    {
        favoriteAll({ "FR" });
        picker()->setActiveUseCaseId(QStringLiteral("favorites"));
        QCOMPARE(picker()->activeUseCaseId(), QStringLiteral("favorites"));

        restore(backupWith(favoritesKey, QJsonObject()));

        QVERIFY(!hasUseCase(picker(), QStringLiteral("favorites")));
        QCOMPARE(picker()->activeUseCaseId(), QStringLiteral("all"));
    }

    void testRestoreOverLimit()
    {
        const QStringList restored = locationCodes.mid(0, picker()->favoritesLimit() + 5);
        QVERIFY(restored.size() < locationCodes.size());

        restore(backupWith(favoritesKey, perServer(serverA, restored)));

        QCOMPARE(picker()->favorites().size(), restored.size());

        QSignalSpy limitSpy(picker(), &ApiCountryListModel::favoritesLimitExceeded);
        QVERIFY(!picker()->toggleFavorite(locationCodes.last()));
        QCOMPARE(limitSpy.count(), 1);

        QVERIFY(picker()->toggleFavorite(restored.first()));
        QCOMPARE(storedFavorites(serverA).size(), restored.size() - 1);
    }

    void testRestoreAtLimitOverSmallerSet()
    {
        // Real-world case: a backup holding exactly the limit restored over a smaller current set
        const QStringList atLimit = { "ge", "hk", "id", "il", "in", "jp", "kr", "kz",
                                      "my", "pe", "ph", "sg", "tr", "us-east", "us-west" };
        QCOMPARE(atLimit.size(), picker()->favoritesLimit());

        showServer(serverA, atLimit + QStringList { "de", "fr" });

        for (const QString &code : atLimit.mid(0, 12)) {
            QVERIFY(picker()->toggleFavorite(code));
        }
        picker()->setActiveUseCaseId(QStringLiteral("favorites"));

        restore(backupWith(favoritesKey, perServer(serverA, atLimit)));

        QStringList expected = atLimit;
        expected.sort();
        QCOMPARE(storedFavorites(serverA), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);
        QCOMPARE(picker()->activeUseCaseId(), QStringLiteral("favorites"));

        QSignalSpy limitSpy(picker(), &ApiCountryListModel::favoritesLimitExceeded);
        QVERIFY(!picker()->toggleFavorite("de"));
        QCOMPARE(limitSpy.count(), 1);
        QCOMPARE(storedFavorites(serverA), expected);
    }

    void testMigrateLegacyList()
    {
        SecureAppSettingsRepository *repository = m_coreController->m_appSettingsRepository;

        m_settings->setValue(favoritesKey, QStringList { "US", "", "DE", "US" });
        repository->migrateLegacyFavoriteLocations({ serverA, serverB });
        QCOMPARE(storedFavorites(serverA), QStringList({ "DE", "US" }));
        QCOMPARE(storedFavorites(serverB), QStringList({ "DE", "US" }));

        repository->setFavoriteLocations(serverB, { "FR" });
        repository->migrateLegacyFavoriteLocations({ serverA, serverB });
        QCOMPARE(storedFavorites(serverB), QStringList({ "FR" }));

        m_settings->setValue(favoritesKey, QStringLiteral("US"));
        repository->migrateLegacyFavoriteLocations({});
        QVERIFY(m_settings->value(favoritesKey).toMap().isEmpty());
        QCOMPARE(m_settings->value(favoritesKey).typeId(), int(QMetaType::QVariantMap));
    }

    void testRestoreLegacyListCopiesToApiServers()
    {
        storeServers({ apiServerJson(serverA, "a.example"), apiServerJson(serverB, "b.example"),
                       selfHostedServerJson(serverSelfHosted, "10.0.0.1") });
        QCOMPARE(m_coreController->m_serversRepository->serversCount(), 3);

        restore(backupWith(favoritesKey, toJsonArray({ "US", "DE" })));

        const QVariantMap stored = m_settings->value(favoritesKey).toMap();
        QCOMPARE(stored.keys(), QStringList({ serverA, serverB }));
        QCOMPARE(storedFavorites(serverA), QStringList({ "DE", "US" }));
        QCOMPARE(storedFavorites(serverB), QStringList({ "DE", "US" }));
        QCOMPARE(picker()->favorites(), QStringList({ "DE", "US" }));

        QVERIFY(picker()->toggleFavorite("NL"));
        QCOMPARE(storedFavorites(serverA), QStringList({ "DE", "NL", "US" }));
        QCOMPARE(storedFavorites(serverB), QStringList({ "DE", "US" }));
    }

    void testServerRemovedDropsItsFavorites()
    {
        storeServers({ apiServerJson(serverA, "a.example"), apiServerJson(serverB, "b.example") });
        favoriteAll({ "US" });
        showServer(serverB);
        favoriteAll({ "FR" });
        showServer(serverA);

        m_coreController->m_serversRepository->removeServer(serverA);

        QVERIFY(storedFavorites(serverA).isEmpty());
        QCOMPARE(storedFavorites(serverB), QStringList({ "FR" }));
        QVERIFY(countryModel()->serverId().isEmpty());
        QVERIFY(picker()->favorites().isEmpty());
    }

    void testGeneratedServerIdIsPersisted()
    {
        QJsonObject unsupported = apiServerJson(QString(), "future.example");
        unsupported[configKey::formatVersion] = serverConfigUtils::currentConfigFormatVersion + 1;
        storeServers({ apiServerJson(QString(), "no-id.example"), unsupported });

        SecureServersRepository *servers = m_coreController->m_serversRepository;
        QCOMPARE(servers->serversCount(), 1);
        const QString generatedId = servers->serverIdAt(0);
        QVERIFY(!generatedId.isEmpty());

        servers->invalidateCache();
        QCOMPARE(servers->serverIdAt(0), generatedId);

        const QJsonArray stored =
                QJsonDocument::fromJson(m_settings->value(QStringLiteral("Servers/serversList")).toByteArray()).array();
        QCOMPARE(stored.size(), 2);
        QCOMPARE(stored.at(0).toObject().value(configKey::storageServerId).toString(), generatedId);
    }

    void testClearSettingsResetsModels()
    {
        favoriteAll({ "FR", "DE" });

        m_coreController->m_settingsController->clearSettings();

        QVERIFY(storedFavorites(serverA).isEmpty());
        QVERIFY(picker()->favorites().isEmpty());
        QVERIFY(nativeConfigs()->favorites().isEmpty());

        QVERIFY(picker()->toggleFavorite("US"));
        QCOMPARE(storedFavorites(serverA), QStringList({ "US" }));
    }

    void testRestoreSortMode()
    {
        QCOMPARE(picker()->sortMode(), int(ApiCountryListModel::ByRegion));

        restore(backupWith(QStringLiteral("Conf/countryListSortMode/picker"), int(ApiCountryListModel::Alphabetical)));

        QCOMPARE(picker()->sortMode(), int(ApiCountryListModel::Alphabetical));
        QCOMPARE(nativeConfigs()->sortMode(), int(ApiCountryListModel::ByRegion));

        m_coreController->m_appSettingsRepository->clearSettings();
        QCOMPARE(picker()->sortMode(), int(ApiCountryListModel::ByRegion));
    }

    void testRoundTrip()
    {
        favoriteAll({ "US", "DE", "JP" });
        showServer(serverB);
        favoriteAll({ "FR" });
        const QByteArray backup = m_coreController->m_settingsController->backupAppConfig();

        m_coreController->m_appSettingsRepository->clearSettings();
        QVERIFY(picker()->favorites().isEmpty());

        restore(backup);

        QCOMPARE(picker()->favorites(), QStringList({ "FR" }));
        QCOMPARE(storedFavorites(serverA), QStringList({ "DE", "JP", "US" }));
        QCOMPARE(storedFavorites(serverB), QStringList({ "FR" }));
    }
};

QTEST_MAIN(TestFavoriteLocationsRestore)
#include "testFavoriteLocationsRestore.moc"
