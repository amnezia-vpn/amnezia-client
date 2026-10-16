#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

#include "core/utils/constants/apiKeys.h"
#include "ui/models/api/apiDevicesModel.h"

class TestApiDevicesModel : public QObject
{
    Q_OBJECT

private:
    static QJsonObject createDevice(const QString &installationUuid, const QString &sourceType,
                                    const QString &osVersion, const QString &countryCode, const QString &lastDownloaded)
    {
        return { { apiDefs::key::installationUuid, installationUuid },
                 { apiDefs::key::sourceType, sourceType },
                 { apiDefs::key::osVersion, osVersion },
                 { apiDefs::key::serverCountryCode, countryCode },
                 { apiDefs::key::lastDownloaded, lastDownloaded },
                 { apiDefs::key::workerLastUpdated, QStringLiteral("2026-10-01T12:00:00Z") } };
    }

private slots:
    void testEmptyModel()
    {
        ApiDevicesModel model;

        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.rowCount(QModelIndex()), 0);

        const QJsonArray devices { createDevice(QStringLiteral("device-1"), QStringLiteral("gateway_account"),
                                                QStringLiteral("Windows 11"), QStringLiteral("EE"),
                                                QStringLiteral("2026-10-10T12:00:00Z")) };

        model.updateModel(devices, QStringLiteral("device-1"));

        QCOMPARE(model.rowCount(), 1);

        const QModelIndex index = model.index(0, 0);
        QVERIFY(index.isValid());
    }

    void testUpdateModel()
    {
        ApiDevicesModel model;

        const QString currentUuid = QStringLiteral("device-1");

        QJsonArray devices {
            createDevice(QStringLiteral("device-1"), QStringLiteral("gateway_account"), QStringLiteral("Windows 11"), QStringLiteral("EE"), QStringLiteral("2026-10-10T12:00:00Z")),
            createDevice(QStringLiteral("device-2"), QStringLiteral("gateway_account"), QStringLiteral("Android 15"), QStringLiteral("DE"), QStringLiteral("2026-10-11T12:00:00Z"))
        };

        model.updateModel(devices, currentUuid);

        QCOMPARE(model.rowCount(), 2);

        const QModelIndex firstIndex = model.index(0, 0);
        const QModelIndex secondIndex = model.index(1, 0);

        QVERIFY(firstIndex.isValid());
        QVERIFY(secondIndex.isValid());

        QCOMPARE(model.data(firstIndex, ApiDevicesModel::OsVersionRole).toString(), QStringLiteral("Windows 11"));
        QCOMPARE(model.data(firstIndex, ApiDevicesModel::SupportTagRole).toString(), QStringLiteral("device-1"));
        QCOMPARE(model.data(firstIndex, ApiDevicesModel::CountryCodeRole).toString(), QStringLiteral("EE"));

        QCOMPARE(model.data(secondIndex, ApiDevicesModel::OsVersionRole).toString(), QStringLiteral("Android 15"));
        QCOMPARE(model.data(secondIndex, ApiDevicesModel::SupportTagRole).toString(), QStringLiteral("device-2"));
        QCOMPARE(model.data(secondIndex, ApiDevicesModel::CountryCodeRole).toString(), QStringLiteral("DE"));
    }

    void testCurrentDeviceRole()
    {
        ApiDevicesModel model;

        QJsonArray devices {
            createDevice(QStringLiteral("device-1"), QStringLiteral("gateway_account"), QStringLiteral("Windows 11"), QStringLiteral("EE"), QStringLiteral("2026-10-10T12:00:00Z")),
            createDevice(QStringLiteral("device-2"), QStringLiteral("gateway_account"), QStringLiteral("Android 15"), QStringLiteral("DE"), QStringLiteral("2026-10-11T12:00:00Z"))
        };

        model.updateModel(devices, QStringLiteral("device-2"));

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0, 0), ApiDevicesModel::IsCurrentDeviceRole).toBool(), false);
        QCOMPARE(model.data(model.index(1, 0), ApiDevicesModel::IsCurrentDeviceRole).toBool(), true);
    }

    void testLastUpdateRole()
    {
        ApiDevicesModel model;

        const QString lastDownloaded = QStringLiteral("2026-10-10T12:00:00Z");

        model.updateModel(QJsonArray { createDevice(QStringLiteral("device-1"), QStringLiteral("gateway_account"), QStringLiteral("Windows 11"), QStringLiteral("EE"), lastDownloaded) }, QStringLiteral("device-1"));

        const QModelIndex index = model.index(0, 0);
        QVERIFY(index.isValid());

        const QString expected = QDateTime::fromString(lastDownloaded, Qt::ISODate).toLocalTime().toString(QStringLiteral("d MMM yyyy"));

        QCOMPARE(model.data(index, ApiDevicesModel::LastUpdateRole).toString(), expected);
    }

    void testFiltersNonGatewayDevices()
    {
        ApiDevicesModel model;

        QJsonArray devices {
            createDevice(QStringLiteral("gateway-1"), QStringLiteral("gateway_account"), QStringLiteral("Windows 11"), QStringLiteral("EE"), QStringLiteral("2026-10-10T12:00:00Z")),
            createDevice(QStringLiteral("other-1"), QStringLiteral("other_source"), QStringLiteral("Linux"), QStringLiteral("DE"), QStringLiteral("2026-10-11T12:00:00Z")),
            createDevice(QStringLiteral("gateway-2"), QStringLiteral("gateway_account"), QStringLiteral("Android 15"), QStringLiteral("FI"), QStringLiteral("2026-10-12T12:00:00Z"))
        };

        model.updateModel(devices, QStringLiteral("gateway-2"));

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0, 0), ApiDevicesModel::SupportTagRole).toString(), QStringLiteral("gateway-1"));
        QCOMPARE(model.data(model.index(1, 0), ApiDevicesModel::SupportTagRole).toString(), QStringLiteral("gateway-2"));
        QCOMPARE(model.data(model.index(1, 0), ApiDevicesModel::IsCurrentDeviceRole).toBool(), true);
    }

    void testEmptyUpdateClearsModel()
    {
        ApiDevicesModel model;

        QJsonArray devices { createDevice(QStringLiteral("device-1"), QStringLiteral("gateway_account"), QStringLiteral("Windows 11"), QStringLiteral("EE"), QStringLiteral("2026-10-10T12:00:00Z")) };

        model.updateModel(devices, QStringLiteral("device-1"));
        QCOMPARE(model.rowCount(), 1);

        model.updateModel(QJsonArray { }, QString { });

        QCOMPARE(model.rowCount(), 0);
    }

    void testInvalidIndexes()
    {
        ApiDevicesModel model;

        const QModelIndex invalidIndex;
        QVERIFY(!model.data(invalidIndex, ApiDevicesModel::OsVersionRole).isValid());

        QJsonArray devices { createDevice(QStringLiteral("device-1"), QStringLiteral("gateway_account"), QStringLiteral("Windows 11"), QStringLiteral("EE"), QStringLiteral("2026-10-10T12:00:00Z")) };

        model.updateModel(devices, QStringLiteral("device-1"));

        QVERIFY(!model.data(model.index(0, 1), ApiDevicesModel::OsVersionRole).isValid());
        QVERIFY(!model.data(model.index(-1, 0), ApiDevicesModel::OsVersionRole).isValid());
        QVERIFY(!model.data(model.index(1, 0), ApiDevicesModel::OsVersionRole).isValid());
        QVERIFY(!model.data(model.index(0, 0), Qt::DisplayRole).isValid());
    }
};

QTEST_MAIN(TestApiDevicesModel)
#include "testApiDevicesModel.moc"
