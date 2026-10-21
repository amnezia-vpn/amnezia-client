#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "utils/testCoreController.h"
#include "core/utils/constants/apiKeys.h"
#include "secureQSettings.h"
#include "vpnConnection.h"

using namespace amnezia;

namespace
{
    const QString favoritesKey = QStringLiteral("Conf/favoriteLocations");

    const QStringList locationCodes = { "AT", "BE", "CA", "CH", "CZ", "DE", "DK", "ES", "FI", "FR", "GB",
                                        "HU", "IE", "IT", "JP", "NL", "NO", "PL", "PT", "SE", "US" };

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

    QByteArray backupWith(const QString &key, const QJsonValue &value)
    {
        QJsonObject backup;
        backup[key] = value;
        return QJsonDocument(backup).toJson();
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

    ApiCountryListModel *picker() const { return m_coreController->m_apiCountryListModel; }
    ApiCountryListModel *nativeConfigs() const { return m_coreController->m_apiConfigsCountryListModel; }
    QStringList storedFavorites() const { return m_coreController->m_appSettingsRepository->favoriteLocations(); }

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
        m_coreController->m_apiCountryModel->updateModel(countriesJson(locationCodes), QString(), QString());
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;
    }

    void init()
    {
        // Goes through the repository so that settingsCleared reaches the models
        m_coreController->m_appSettingsRepository->clearSettings();
        QVERIFY(picker()->favorites().isEmpty());
        QVERIFY(nativeConfigs()->favorites().isEmpty());
    }

    void testBackupContainsFavorites()
    {
        favoriteAll({ "US", "DE", "NL" });

        const QJsonObject backup = QJsonDocument::fromJson(m_coreController->m_settingsController->backupAppConfig()).object();

        QVERIFY(backup.contains(favoritesKey));
        QCOMPARE(backup.value(favoritesKey).toVariant().toStringList(), QStringList({ "DE", "NL", "US" }));
    }

    void testRestoreReloadsBothModels()
    {
        favoriteAll({ "FR" });

        QSignalSpy pickerChangedSpy(picker(), &ApiCountryListModel::favoritesChanged);
        QSignalSpy nativeChangedSpy(nativeConfigs(), &ApiCountryListModel::favoritesChanged);

        restore(backupWith(favoritesKey, QJsonArray { "US", "DE" }));

        const QStringList expected = { "DE", "US" };
        QCOMPARE(storedFavorites(), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);
        QCOMPARE(pickerChangedSpy.count(), 0);
        QCOMPARE(nativeChangedSpy.count(), 0);
    }

    void testToggleAfterRestoreKeepsRestored()
    {
        favoriteAll({ "FR" });
        restore(backupWith(favoritesKey, QJsonArray { "US", "DE" }));

        QVERIFY(picker()->toggleFavorite("NL"));

        const QStringList expected = { "DE", "NL", "US" };
        QCOMPARE(storedFavorites(), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);

        QVERIFY(nativeConfigs()->toggleFavorite("US"));
        QCOMPARE(storedFavorites(), QStringList({ "DE", "NL" }));
        QCOMPARE(picker()->favorites(), QStringList({ "DE", "NL" }));
    }

    void testRestoreWithoutKeyKeepsCurrent()
    {
        favoriteAll({ "FR" });

        restore(backupWith(QStringLiteral("Conf/autoConnect"), false));

        QCOMPARE(storedFavorites(), QStringList({ "FR" }));
        QCOMPARE(picker()->favorites(), QStringList({ "FR" }));
        QCOMPARE(nativeConfigs()->favorites(), QStringList({ "FR" }));
    }

    void testRestoreValueShapes_data()
    {
        QTest::addColumn<QJsonValue>("value");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("empty array") << QJsonValue(QJsonArray()) << QStringList();
        QTest::newRow("null") << QJsonValue(QJsonValue::Null) << QStringList();
        QTest::newRow("single string") << QJsonValue(QStringLiteral("US")) << QStringList({ "US" });
        QTest::newRow("duplicates and empty") << QJsonValue(QJsonArray { "US", "US", "", "DE" }) << QStringList({ "DE", "US" });
        QTest::newRow("number") << QJsonValue(42) << QStringList();
        QTest::newRow("bool") << QJsonValue(true) << QStringList();
        QTest::newRow("object") << QJsonValue(QJsonObject()) << QStringList();
    }

    void testRestoreValueShapes()
    {
        QFETCH(QJsonValue, value);
        QFETCH(QStringList, expected);

        favoriteAll({ "FR" });

        restore(backupWith(favoritesKey, value));

        QCOMPARE(storedFavorites(), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);
    }

    void testRestoreEmptyResetsFavoritesChip()
    {
        favoriteAll({ "FR" });
        picker()->setActiveUseCaseId(QStringLiteral("favorites"));
        QCOMPARE(picker()->activeUseCaseId(), QStringLiteral("favorites"));

        restore(backupWith(favoritesKey, QJsonArray()));

        QVERIFY(!hasUseCase(picker(), QStringLiteral("favorites")));
        QCOMPARE(picker()->activeUseCaseId(), QStringLiteral("all"));
    }

    void testRestoreOverLimit()
    {
        const QStringList restored = locationCodes.mid(0, picker()->favoritesLimit() + 5);
        QVERIFY(restored.size() < locationCodes.size());

        QJsonArray array;
        for (const QString &code : restored) {
            array.append(code);
        }
        restore(backupWith(favoritesKey, array));

        QCOMPARE(picker()->favorites().size(), restored.size());

        QSignalSpy limitSpy(picker(), &ApiCountryListModel::favoritesLimitExceeded);
        QVERIFY(!picker()->toggleFavorite(locationCodes.last()));
        QCOMPARE(limitSpy.count(), 1);

        QVERIFY(picker()->toggleFavorite(restored.first()));
        QCOMPARE(storedFavorites().size(), restored.size() - 1);
    }

    void testRestoreAtLimitOverSmallerSet()
    {
        // Real-world case: a backup holding exactly the limit restored over a smaller current set
        const QStringList atLimit = { "ge", "hk", "id", "il", "in", "jp", "kr", "kz",
                                      "my", "pe", "ph", "sg", "tr", "us-east", "us-west" };
        QCOMPARE(atLimit.size(), picker()->favoritesLimit());

        m_coreController->m_apiCountryModel->updateModel(countriesJson(atLimit + QStringList { "de", "fr" }), QString(), QString());

        for (const QString &code : atLimit.mid(0, 12)) {
            QVERIFY(picker()->toggleFavorite(code));
        }
        picker()->setActiveUseCaseId(QStringLiteral("favorites"));

        QJsonArray array;
        for (const QString &code : atLimit) {
            array.append(code);
        }
        restore(backupWith(favoritesKey, array));

        QStringList expected = atLimit;
        expected.sort();
        QCOMPARE(storedFavorites(), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);
        QCOMPARE(picker()->activeUseCaseId(), QStringLiteral("favorites"));

        QSignalSpy limitSpy(picker(), &ApiCountryListModel::favoritesLimitExceeded);
        QVERIFY(!picker()->toggleFavorite("de"));
        QCOMPARE(limitSpy.count(), 1);
        QCOMPARE(storedFavorites(), expected);

        m_coreController->m_apiCountryModel->updateModel(countriesJson(locationCodes), QString(), QString());
    }

    void testRestoreRealBackupsSequence()
    {
        const QString newBackupPath = qEnvironmentVariable("FAVORITES_NEW_BACKUP_PATH");
        const QString oldBackupPath = qEnvironmentVariable("FAVORITES_OLD_BACKUP_PATH");
        if (newBackupPath.isEmpty() || oldBackupPath.isEmpty()) {
            QSKIP("Set FAVORITES_NEW_BACKUP_PATH and FAVORITES_OLD_BACKUP_PATH");
        }

        auto readFile = [](const QString &path) {
            QFile file(path);
            return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
        };
        const QByteArray newBackup = readFile(newBackupPath);
        const QByteArray oldBackup = readFile(oldBackupPath);
        QVERIFY(!newBackup.isEmpty() && !oldBackup.isEmpty());

        restore(newBackup);
        m_coreController->m_serversUiController->setProcessedServerId(m_coreController->m_serversController->getDefaultServerId());
        qInfo() << "locations after new restore:" << m_coreController->m_apiCountryModel->rowCount()
                << "favorites:" << picker()->favorites();

        restore(oldBackup);
        qInfo() << "locations after old restore:" << m_coreController->m_apiCountryModel->rowCount()
                << "favorites:" << picker()->favorites();

        const QStringList expected = QJsonDocument::fromJson(oldBackup).object().value(favoritesKey).toVariant().toStringList();
        QCOMPARE(picker()->favorites(), expected);

        m_coreController->m_apiCountryModel->updateModel(countriesJson(locationCodes), QString(), QString());
    }

    void testClearSettingsResetsModels()
    {
        favoriteAll({ "FR", "DE" });

        m_coreController->m_settingsController->clearSettings();

        QVERIFY(storedFavorites().isEmpty());
        QVERIFY(picker()->favorites().isEmpty());
        QVERIFY(nativeConfigs()->favorites().isEmpty());

        QVERIFY(picker()->toggleFavorite("US"));
        QCOMPARE(storedFavorites(), QStringList({ "US" }));
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
        const QStringList expected = picker()->favorites();
        const QByteArray backup = m_coreController->m_settingsController->backupAppConfig();

        m_coreController->m_appSettingsRepository->clearSettings();
        QVERIFY(picker()->favorites().isEmpty());

        restore(backup);

        QCOMPARE(storedFavorites(), expected);
        QCOMPARE(picker()->favorites(), expected);
        QCOMPARE(nativeConfigs()->favorites(), expected);
    }
};

QTEST_MAIN(TestFavoriteLocationsRestore)
#include "testFavoriteLocationsRestore.moc"
