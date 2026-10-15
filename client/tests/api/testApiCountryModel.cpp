#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

#include "core/utils/constants/apiKeys.h"
#include "ui/models/api/apiCountryModel.h"

        class TestApiCountryModel : public QObject
{
    Q_OBJECT

private:
    ApiCountryModel m_model;

    static QJsonObject makeCountry(const QString &name, const QString &code, const QString &localizedCode)
    {
        return { { apiDefs::key::serverCountryName, name },
                 { apiDefs::key::serverCountryCode, code },
                 { apiDefs::key::serverCountryCodeL10n, localizedCode } };
    }

    static QJsonObject makeIssuedConfig(const QString &countryCode, const QString &sourceType,
                                        const QString &lastDownloaded, const QString &workerLastUpdated)
    {
        return { { apiDefs::key::serverCountryCode, countryCode },
                 { apiDefs::key::sourceType, sourceType },
                 { apiDefs::key::lastDownloaded, lastDownloaded },
                 { apiDefs::key::workerLastUpdated, workerLastUpdated },
                 { apiDefs::key::installationUuid, "test-installation" },
                 { apiDefs::key::osVersion, "test-os" } };
    }

private slots:
    void init()
    {
        m_model.updateIssuedConfigsInfo(QJsonArray {});
        m_model.updateModel(QJsonArray {}, QString {}, QString {});
    }

    void testInitialState()
    {
        QCOMPARE(m_model.rowCount(), 0);
        QCOMPARE(m_model.getCurrentIndex(), -1);
        QVERIFY(m_model.getCurrentCountryCode().isEmpty());
        QVERIFY(m_model.getUserCountryCode().isEmpty());
        QVERIFY(!m_model.hasExpiredWorkerConfigs());
        QVERIFY(!m_model.data(m_model.index(0, 0), ApiCountryModel::CountryNameRole).isValid());
    }

    void testUpdateModel()
    {
        const QJsonArray countries { makeCountry("Germany", "DE", "de"), makeCountry("Estonia", "EE", "ee"),
                                     makeCountry("United States", "US", "us") };

        QSignalSpy currentIndexSpy(&m_model, &ApiCountryModel::currentIndexChanged);

        m_model.updateModel(countries, "EE", "us");

        QCOMPARE(m_model.rowCount(), 3);
        QCOMPARE(m_model.getCurrentIndex(), 1);
        QCOMPARE(m_model.getCurrentCountryCode(), QString("EE"));
        QCOMPARE(m_model.getUserCountryCode(), QString("US"));

        QCOMPARE(currentIndexSpy.count(), 1);
        QCOMPARE(currentIndexSpy.constFirst().constFirst().toInt(), 1);

        const QModelIndex germanyIndex = m_model.index(0, 0);
        const QModelIndex estoniaIndex = m_model.index(1, 0);
        const QModelIndex unitedStatesIndex = m_model.index(2, 0);

        QVERIFY(germanyIndex.isValid());
        QCOMPARE(m_model.data(germanyIndex, ApiCountryModel::CountryNameRole).toString(), QString("Germany"));
        QCOMPARE(m_model.data(germanyIndex, ApiCountryModel::CountryCodeRole).toString(), QString("DE"));

        QCOMPARE(m_model.data(estoniaIndex, ApiCountryModel::CountryNameRole).toString(), QString("Estonia"));
        QCOMPARE(m_model.data(estoniaIndex, ApiCountryModel::CountryCodeRole).toString(), QString("EE"));

        QCOMPARE(m_model.data(unitedStatesIndex, ApiCountryModel::CountryNameRole).toString(), QString("United States"));
        QCOMPARE(m_model.data(unitedStatesIndex, ApiCountryModel::CountryCodeRole).toString(), QString("US"));
    }

    void testCurrentCountryNotFound()
    {
        const QJsonArray countries { makeCountry("Germany", "DE", "de"), makeCountry("Estonia", "EE", "ee") };

        m_model.updateModel(countries, "FI", "ee");

        QCOMPARE(m_model.rowCount(), 2);
        QCOMPARE(m_model.getCurrentIndex(), -1);
        QCOMPARE(m_model.getCurrentCountryCode(), QString("FI"));
        QCOMPARE(m_model.getUserCountryCode(), QString("EE"));
    }

    void testCountryImageCodeRole()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee") }, "EE", "EE");

        const QModelIndex index = m_model.index(0, 0);

        QCOMPARE(m_model.data(index, ApiCountryModel::CountryImageCodeRole).toString(), QString("EE"));
    }

    void testCountryImageCodeFallback()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "") }, "EE", "EE");

        QCOMPARE(m_model.data(m_model.index(0, 0), ApiCountryModel::CountryImageCodeRole).toString(), QString("EE"));
    }

    void testInvalidIndexesAndRoles()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee") }, "EE", "EE");

        QVERIFY(!m_model.data(QModelIndex(), ApiCountryModel::CountryNameRole).isValid());
        QVERIFY(!m_model.data(m_model.index(-1, 0), ApiCountryModel::CountryNameRole).isValid());
        QVERIFY(!m_model.data(m_model.index(1, 0), ApiCountryModel::CountryNameRole).isValid());
        QVERIFY(!m_model.data(m_model.index(0, 0), Qt::UserRole + 100).isValid());
    }

    void testRoleNames()
    {
        const auto roles = m_model.roleNames();

        QCOMPARE(roles.value(ApiCountryModel::CountryNameRole), QByteArray("countryName"));
        QCOMPARE(roles.value(ApiCountryModel::CountryCodeRole), QByteArray("countryCode"));
        QCOMPARE(roles.value(ApiCountryModel::CountryImageCodeRole), QByteArray("countryImageCode"));
        QCOMPARE(roles.value(ApiCountryModel::IsIssuedRole), QByteArray("isIssued"));
        QCOMPARE(roles.value(ApiCountryModel::IsWorkerExpiredRole), QByteArray("isWorkerExpired"));
    }

    void testUpdateIssuedConfigsInfo()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee"), makeCountry("Germany", "DE", "de") }, "EE",
                            "EE");

        const QJsonArray issuedConfigs {
            // Worker timestamp equals download timestamp: not expired.
            makeIssuedConfig("EE", "country_config", "2026-10-10T10:00:00", "2026-10-10T10:00:00"),

            // Worker timestamp is newer than the download timestamp: expired.
            makeIssuedConfig("DE", "country_config", "2026-10-09T10:00:00", "2026-10-10T10:00:00"),

            // Non-country configurations must be ignored.
            makeIssuedConfig("EE", "other_config", "2026-10-01T10:00:00", "2026-10-20T10:00:00")
        };

        m_model.updateIssuedConfigsInfo(issuedConfigs);

        const QModelIndex estoniaIndex = m_model.index(0, 0);
        const QModelIndex germanyIndex = m_model.index(1, 0);

        QVERIFY(m_model.data(estoniaIndex, ApiCountryModel::IsIssuedRole).toBool());
        QVERIFY(!m_model.data(estoniaIndex, ApiCountryModel::IsWorkerExpiredRole).toBool());

        QVERIFY(m_model.data(germanyIndex, ApiCountryModel::IsIssuedRole).toBool());
        QVERIFY(m_model.data(germanyIndex, ApiCountryModel::IsWorkerExpiredRole).toBool());

        QVERIFY(m_model.hasExpiredWorkerConfigs());
    }

    void testNonCountryConfigIsIgnored()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee") }, "EE", "EE");

        m_model.updateIssuedConfigsInfo(QJsonArray { makeIssuedConfig("EE", "other_config", "2026-10-01T10:00:00", "2026-10-20T10:00:00") });

        const QModelIndex index = m_model.index(0, 0);

        QVERIFY(!m_model.data(index, ApiCountryModel::IsIssuedRole).toBool());
        QVERIFY(!m_model.data(index, ApiCountryModel::IsWorkerExpiredRole).toBool());
        QVERIFY(!m_model.hasExpiredWorkerConfigs());
    }

    void testCountryWithoutIssuedConfig()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee") }, "EE", "EE");

        m_model.updateIssuedConfigsInfo(QJsonArray { });

        const QModelIndex index = m_model.index(0, 0);

        QVERIFY(!m_model.data(index, ApiCountryModel::IsIssuedRole).toBool());
        QVERIFY(!m_model.data(index, ApiCountryModel::IsWorkerExpiredRole).toBool());
        QVERIFY(!m_model.hasExpiredWorkerConfigs());
    }

    void testUpdateModelResetsCountries()
    {
        m_model.updateModel(QJsonArray { makeCountry("Germany", "DE", "de"), makeCountry("Estonia", "EE", "ee") }, "EE", "EE");
        m_model.updateModel(QJsonArray { makeCountry("Finland", "FI", "fi") }, "FI", "FI");

        QCOMPARE(m_model.rowCount(), 1);
        QCOMPARE(m_model.getCurrentIndex(), 0);
        QCOMPARE(m_model.getCurrentCountryCode(), QString("FI"));
        QCOMPARE(m_model.getUserCountryCode(), QString("FI"));
        QCOMPARE(m_model.data(m_model.index(0, 0), ApiCountryModel::CountryNameRole).toString(), QString("Finland"));
    }
};

QTEST_MAIN(TestApiCountryModel)
#include "testApiCountryModel.moc"