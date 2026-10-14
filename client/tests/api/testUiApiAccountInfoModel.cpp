#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "ui/models/api/apiAccountInfoModel.h"

using namespace amnezia;

namespace
{

    QJsonObject makeAccountInfo()
    {
        return { { apiDefs::key::activeDeviceCount, 2 },
                 { apiDefs::key::maxDeviceCount, 5 },
                 { apiDefs::key::subscriptionEndDate, "2099-12-31T23:59:59Z" },
                 { apiDefs::key::subscriptionDescription, "Premium subscription" },
                 { apiDefs::key::isRenewalAvailable, true },
                 { apiDefs::key::availableCountries, QJsonArray { "ru", "de" } },
                 { apiDefs::key::issuedConfigs, QJsonArray { } },
                 { apiDefs::key::supportInfo,
                   QJsonObject { { apiDefs::key::telegram, "https://t.me/test_support" },
                                 { apiDefs::key::email, "support@example.org" },
                                 { apiDefs::key::billingEmail, "billing@example.org" },
                                 { apiDefs::key::websiteName, "Example" },
                                 { apiDefs::key::website, "https://example.org" } } } };
    }

    QJsonObject makePremiumServerConfig()
    {
        QJsonObject apiConfig;
        apiConfig[apiDefs::key::isInAppPurchase] = false;

        QJsonObject config;
        config[configKey::configVersion] = serverConfigUtils::ConfigSource::AmneziaGateway;
        config[apiDefs::key::apiConfig] = apiConfig;

        return config;
    }

} // namespace

class TestUiApiAccountInfoModel : public QObject
{
    Q_OBJECT

private slots:
    void testInitialState()
    {
        ApiAccountInfoModel model;

        QCOMPARE(model.rowCount(), 1);
        QVERIFY(!model.data(QModelIndex(), ApiAccountInfoModel::SubscriptionStatusRole).isValid());

        const QModelIndex invalidIndex = model.index(1, 0);
        QVERIFY(!model.data(invalidIndex, ApiAccountInfoModel::SubscriptionStatusRole).isValid());
    }

    void testUpdateModel()
    {
        ApiAccountInfoModel model;

        QSignalSpy modelResetSpy(&model, &QAbstractItemModel::modelReset);

        model.updateModel(makeAccountInfo(), makePremiumServerConfig());

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(modelResetSpy.count(), 1);

        const QModelIndex index = model.index(0, 0);
        QVERIFY(index.isValid());

        QCOMPARE(model.data(index, ApiAccountInfoModel::ConnectedDevicesRole).toString(), QStringLiteral("2 out of 5"));
        QCOMPARE(model.data(index, ApiAccountInfoModel::ServiceDescriptionRole).toString(), QStringLiteral("Premium subscription"));
        QCOMPARE(model.data(index, ApiAccountInfoModel::IsSubscriptionRenewalAvailableRole).toBool(), true);
        QCOMPARE(model.data(index, ApiAccountInfoModel::IsInAppPurchaseRole).toBool(), false);
        QCOMPARE(model.data(index, ApiAccountInfoModel::IsComponentVisibleRole).toBool(), false);
        QCOMPARE(model.data(QStringLiteral("serviceDescription")).toString(), QStringLiteral("Premium subscription"));
    }

    void testSupportLinks()
    {
        ApiAccountInfoModel model;
        model.updateModel(makeAccountInfo(), makePremiumServerConfig());

        QCOMPARE(model.getTelegramBotLink(), QStringLiteral("https://t.me/test_support"));
        QCOMPARE(model.getEmailLink(), QStringLiteral("support@example.org"));
        QCOMPARE(model.getBillingEmailLink(), QStringLiteral("billing@example.org"));
        QCOMPARE(model.getSiteLink(), QStringLiteral("Example"));
        QCOMPARE(model.getFullSiteLink(), QStringLiteral("https://example.org"));
    }

    void testAvailableCountriesAndIssuedConfigs()
    {
        ApiAccountInfoModel model;
        model.updateModel(makeAccountInfo(), makePremiumServerConfig());

        QCOMPARE(model.getAvailableCountries(), QJsonArray({ "ru", "de" }));
        QCOMPARE(model.getIssuedConfigsInfo(), QJsonArray { });
    }

    void testMissingOptionalFields()
    {
        ApiAccountInfoModel model;
        model.updateModel(QJsonObject { }, makePremiumServerConfig());

        const QModelIndex index = model.index(0, 0);
        QVERIFY(index.isValid());

        QCOMPARE(model.data(index, ApiAccountInfoModel::ConnectedDevicesRole).toString(), QStringLiteral("0 out of 0"));
        QCOMPARE(model.data(index, ApiAccountInfoModel::ServiceDescriptionRole).toString(), QString());
        QCOMPARE(model.data(index, ApiAccountInfoModel::IsSubscriptionRenewalAvailableRole).toBool(), false);
        QCOMPARE(model.data(index, ApiAccountInfoModel::IsInAppPurchaseRole).toBool(), false);

        QVERIFY(model.getTelegramBotLink().isEmpty());
        QVERIFY(model.getEmailLink().isEmpty());
        QVERIFY(model.getBillingEmailLink().isEmpty());
        QVERIFY(model.getSiteLink().isEmpty());
        QVERIFY(model.getFullSiteLink().isEmpty());
    }

    void testUpdateModelResetsData()
    {
        ApiAccountInfoModel model;
        model.updateModel(makeAccountInfo(), makePremiumServerConfig());

        QCOMPARE(model.data(model.index(0, 0), ApiAccountInfoModel::ConnectedDevicesRole).toString(), QStringLiteral("2 out of 5"));

        QJsonObject updatedInfo = makeAccountInfo();
        updatedInfo[apiDefs::key::activeDeviceCount] = 1;
        updatedInfo[apiDefs::key::maxDeviceCount] = 3;
        updatedInfo[apiDefs::key::subscriptionDescription] = "Updated subscription";

        model.updateModel(updatedInfo, makePremiumServerConfig());

        QCOMPARE(model.data(model.index(0, 0), ApiAccountInfoModel::ConnectedDevicesRole).toString(), QStringLiteral("1 out of 3"));
        QCOMPARE(model.data(model.index(0, 0), ApiAccountInfoModel::ServiceDescriptionRole).toString(), QStringLiteral("Updated subscription"));
    }
};

QTEST_MAIN(TestUiApiAccountInfoModel)
#include "testUiApiAccountInfoModel.moc"