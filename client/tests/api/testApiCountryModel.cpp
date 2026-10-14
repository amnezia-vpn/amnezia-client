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
        m_model.updateIssuedConfigsInfo(QJsonArray { });
        m_model.updateModel(QJsonArray { }, QString { });
    }

    void testInitialState()
    {
        QCOMPARE(m_model.rowCount(), 0);
        QVERIFY(!m_model.data(m_model.index(0, 0), ApiCountryModel::CountryNameRole).isValid());
    }

    void testUpdateModel()
    {
        const QJsonArray countries { makeCountry("Germany", "DE", "de"),
                                     makeCountry("Estonia", "EE", "ee"),
                                     makeCountry("United States", "US", "us") };

        QSignalSpy currentIndexSpy(&m_model, &ApiCountryModel::currentIndexChanged);

        m_model.updateModel(countries, "EE");

        QCOMPARE(m_model.rowCount(), 3);
        QCOMPARE(m_model.getCurrentIndex(), 1);
        QCOMPARE(currentIndexSpy.count(), 1);
        QCOMPARE(currentIndexSpy.constFirst().constFirst().toInt(), 1);

        const QModelIndex index = m_model.index(0, 0);
        QVERIFY(index.isValid());

        QCOMPARE(m_model.data(index, ApiCountryModel::CountryNameRole).toString(), QString("Germany"));
        QCOMPARE(m_model.data(index, ApiCountryModel::CountryCodeRole).toString(), QString("DE"));

        QCOMPARE(m_model.data(m_model.index(1, 0), ApiCountryModel::CountryNameRole).toString(), QString("Estonia"));
        QCOMPARE(m_model.data(m_model.index(2, 0), ApiCountryModel::CountryCodeRole).toString(), QString("US"));
    }

    void testCountryImageCodeRole()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee") }, "EE");

        const QVariant imageCode = m_model.data(m_model.index(0, 0), ApiCountryModel::CountryImageCodeRole);

        QVERIFY(imageCode.isValid());
        QVERIFY(!imageCode.toString().isEmpty());
    }

    void testInvalidIndexesAndRoles()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee") }, "EE");

        QVERIFY(!m_model.data(QModelIndex(), ApiCountryModel::CountryNameRole).isValid());
        QVERIFY(!m_model.data(m_model.index(-1, 0), ApiCountryModel::CountryNameRole).isValid());
        QVERIFY(!m_model.data(m_model.index(1, 0), ApiCountryModel::CountryNameRole).isValid());
        QVERIFY(!m_model.data(m_model.index(0, 0), Qt::UserRole + 100).isValid());
    }

    void testUpdateIssuedConfigsInfo()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee"), makeCountry("Germany", "DE", "de") }, "EE");

        const QJsonArray issuedConfigs {
            makeIssuedConfig("EE", "country_config", "2026-10-10T10:00:00", "2026-10-10T10:00:00"),
            makeIssuedConfig("DE", "country_config", "2026-10-09T10:00:00", "2026-10-10T10:00:00"),
            makeIssuedConfig("EE", "other_config", "2026-10-01T10:00:00", "2026-10-20T10:00:00")
        };

        m_model.updateIssuedConfigsInfo(issuedConfigs);

        const QModelIndex estoniaIndex = m_model.index(0, 0);
        const QModelIndex germanyIndex = m_model.index(1, 0);

        QCOMPARE(m_model.data(estoniaIndex, ApiCountryModel::IsIssuedRole).toBool(), true);
        QCOMPARE(m_model.data(estoniaIndex, ApiCountryModel::IsWorkerExpiredRole).toBool(), false);

        QCOMPARE(m_model.data(germanyIndex, ApiCountryModel::IsIssuedRole).toBool(), true);
        QCOMPARE(m_model.data(germanyIndex, ApiCountryModel::IsWorkerExpiredRole).toBool(), true);
    }

    void testCountryWithoutIssuedConfig()
    {
        m_model.updateModel(QJsonArray { makeCountry("Estonia", "EE", "ee") }, "EE");
        m_model.updateIssuedConfigsInfo(QJsonArray {});

        const QModelIndex index = m_model.index(0, 0);

        QCOMPARE(m_model.data(index, ApiCountryModel::IsIssuedRole).toBool(), false);
        QCOMPARE(m_model.data(index, ApiCountryModel::IsWorkerExpiredRole).toBool(), false);
    }

    void testSetCurrentIndex()
    {
        m_model.updateModel(QJsonArray { makeCountry("Germany", "DE", "de"), makeCountry("Estonia", "EE", "ee") }, "DE");

        QSignalSpy currentIndexSpy(&m_model, &ApiCountryModel::currentIndexChanged);

        m_model.setCurrentIndex(1);

        QCOMPARE(m_model.getCurrentIndex(), 1);
        QCOMPARE(currentIndexSpy.count(), 1);
        QCOMPARE(currentIndexSpy.constFirst().constFirst().toInt(), 1);
    }

    void testUpdateModelResetsCountries()
    {
        m_model.updateModel(QJsonArray { makeCountry("Germany", "DE", "de"), makeCountry("Estonia", "EE", "ee") }, "EE");
        m_model.updateModel(QJsonArray { makeCountry("Finland", "FI", "fi") }, "FI");

        QCOMPARE(m_model.rowCount(), 1);
        QCOMPARE(m_model.getCurrentIndex(), 0);
        QCOMPARE(m_model.data(m_model.index(0, 0), ApiCountryModel::CountryNameRole).toString(), QString("Finland"));
    }
};

QTEST_MAIN(TestApiCountryModel)
#include "testApiCountryModel.moc"
