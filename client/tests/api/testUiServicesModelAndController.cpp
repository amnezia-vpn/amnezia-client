#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "utils/testCoreController.h"
#include "utils/testUtils.h"

#include "secureQSettings.h"
#include "vpnConnection.h"

#include "ui/models/api/apiServicesModel.h"

using namespace amnezia;
using namespace amnezia::test;

namespace
{

QJsonObject makeServiceItem(const QString &name,
                            const QString &cardDescription,
                            const QString &description,
                            const QString &type,
                            const QString &protocol,
                            const bool isAvailable,
                            const QString &price,
                            const QString &endDate,
                            const QString &termsOfUseUrl,
                            const QString &privacyPolicyUrl,
                            const QJsonArray &subscriptionPlans = {})
{
    return {
        { "service_type", type },
        { "service_protocol", protocol },
        { "is_available", isAvailable },
        { "service_description",
          QJsonObject {
              { "service_name", name },
              { "card_description", cardDescription },
              { "description", description },
              { "terms_of_use_url", termsOfUseUrl },
              { "privacy_policy_url", privacyPolicyUrl },
              { "subscription_plans", subscriptionPlans },
              { "min_price_label", price }
          } },
        { "subscription",
          QJsonObject {
              { "end_date", endDate }
          } },
        { "available_countries",
          QJsonArray {
              "EE",
              "FI"
          } },
        { "store_endpoint", "https://example.com/store" }
    };
}

QJsonObject makeServicesResponse()
{
    return {
        { "user_country_code", "EE" },
        { "services",
          QJsonArray {
              makeServiceItem("Amnezia Premium",
                              "Premium service",
                              "Premium service description",
                              "amnezia-premium",
                              "xray",
                              true,
                              "$19.99",
                              "2026-10-29T10:00:00Z",
                              "https://example.com/terms",
                              "https://example.com/privacy",
                              QJsonArray {
                                  QJsonObject {
                                      { "id", "monthly" },
                                      { "price", "$19.99" }
                                  }
                              }),

              makeServiceItem("Amnezia Free",
                              "Free service",
                              "Free service description",
                              "amnezia-free",
                              "wireguard",
                              true,
                              "$0",
                              "",
                              "https://example.com/terms",
                              "https://example.com/privacy")
          } }
    };
}

} // namespace

class TestUiServicesModelAndController : public QObject
{
    Q_OBJECT

private:
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;

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
        m_coreController->m_apiServicesModel->updateModel(QJsonObject {});
    }

    void testRolesAndSignals()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;

        QSignalSpy selectionChangedSpy(model, &ApiServicesModel::serviceSelectionChanged);

        model->updateModel(makeServicesResponse());

        QCOMPARE(model->rowCount(), 3);
        QCOMPARE(selectionChangedSpy.count(), 1);

        const QModelIndex firstIndex = model->index(0, 0);

        QCOMPARE(model->data(firstIndex, ApiServicesModel::NameRole).toString(), QString("service-1"));
        QCOMPARE(model->data(firstIndex, ApiServicesModel::CardDescriptionRole).toString(), QString("First service"));
        QCOMPARE(model->data(firstIndex, ApiServicesModel::ServiceDescriptionRole).toString(), QString("First service description"));
        QCOMPARE(model->data(firstIndex, ApiServicesModel::IsServiceAvailableRole).toBool(), true);
        QCOMPARE(model->data(firstIndex, ApiServicesModel::IsPremiumRole).toBool(), false);
        QCOMPARE(model->data(firstIndex, ApiServicesModel::HasSubscriptionPlansRole).toBool(), true);
        QCOMPARE(model->data(firstIndex, ApiServicesModel::PriceRole).toString(), QString("$9.99"));
        QCOMPARE(model->data(firstIndex, ApiServicesModel::TermsOfUseUrlRole).toString(),QString("https://example.com/terms"));
        QCOMPARE(model->data(firstIndex, ApiServicesModel::PrivacyPolicyUrlRole).toString(), QString("https://example.com/privacy"));
        QCOMPARE(model->data(firstIndex, ApiServicesModel::ShowRecommendedRole).toBool(), false);
        QCOMPARE(model->data(firstIndex, ApiServicesModel::OrderRole).toInt(), 0);
    }

    void testServiceSelection()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;
        model->updateModel(makeServicesResponse());

        QCOMPARE(model->serviceIndexForType("service-1"), 0);
        QCOMPARE(model->serviceIndexForType("service-2"), 1);
        QCOMPARE(model->serviceIndexForType("unknown"), -1);

        QSignalSpy selectionChangedSpy(model, &ApiServicesModel::serviceSelectionChanged);

        model->setServiceIndex(1);

        QCOMPARE(selectionChangedSpy.count(), 1);
        QCOMPARE(model->getSelectedServiceName(), QString("service-2"));
        QCOMPARE(model->getSelectedServiceType(), QString("service-2"));
        QCOMPARE(model->getSelectedServiceProtocol(), QString("service-2"));
        QCOMPARE(model->getSelectedServiceCountries(), QJsonArray());
        QCOMPARE(model->getCountryCode(), QString());
        QCOMPARE(model->getStoreEndpoint(), QString());
    }

    void testSelectedServiceData()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;
        model->updateModel(makeServicesResponse());

        QCOMPARE(model->getSelectedServiceData("name").toString(), QString("service-1"));
        QCOMPARE(model->getSelectedServiceData("cardDescription").toString(), QString("First service"));
        QCOMPARE(model->getSelectedServiceData("serviceDescription").toString(), QString("First service description"));
        QCOMPARE(model->getSelectedServiceData("isServiceAvailable").toBool(), true);
        QCOMPARE(model->getSelectedServiceData("isPremium").toBool(), false);
        QCOMPARE(model->getSelectedServiceData("hasSubscriptionPlans").toBool(), true);
        QCOMPARE(model->getSelectedServiceData("price").toString(), QString("$9.99"));
        QCOMPARE(model->getSelectedServiceData("termsOfUseUrl").toString(), QString("https://example.com/terms"));
        QCOMPARE(model->getSelectedServiceData("privacyPolicyUrl").toString(), QString("https://example.com/privacy"));
        QVERIFY(!model->getSelectedServiceData("unknownRole").isValid());
    }

    void testEndDate()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;
        model->updateModel(makeServicesResponse());

        const QString endDate = model->data(model->index(0, 0), ApiServicesModel::EndDateRole).toString();
        const QString expected = QDateTime::fromString("2026-10-29T10:00:00Z", Qt::ISODate).toLocalTime().toString("d MMM yyyy");

        QCOMPARE(endDate, expected);
    }

    void testFreeServiceUnavailable()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;

        QJsonArray services { makeServiceItem("service-3", "Third service", "Third service description", "false",
                                              "false", "false", "$4.99", "2026-09-27T10:00:00Z",
                                              "https://example.com/terms", "https://example.com/privacy", "false", "3") };

        QJsonObject data { { "services", services } };

        model->updateModel(data);

        const QModelIndex index = model->index(0, 0);

        QCOMPARE(model->data(index, ApiServicesModel::IsServiceAvailableRole).toBool(), false);
        QCOMPARE(model->data(index, ApiServicesModel::IsPremiumRole).toBool(), false);
        QCOMPARE(model->data(index, ApiServicesModel::ShowRecommendedRole).toBool(), false);
        QCOMPARE(model->data(index, ApiServicesModel::OrderRole).toInt(), 1);

        const QString description = model->data(index, ApiServicesModel::CardDescriptionRole).toString();

        QVERIFY(description.contains("Not available in your region"));
    }

    void testInvalidModelIndex()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;
        model->updateModel(makeServicesResponse());

        QVERIFY(!model->data(QModelIndex(), ApiServicesModel::NameRole).isValid());
        QVERIFY(!model->data(model->index(-1, 0), ApiServicesModel::NameRole).isValid());
        QVERIFY(!model->data(model->index(model->rowCount(), 0), ApiServicesModel::NameRole).isValid());
    }

    void testEmptyResponse()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;

        QSignalSpy selectionChangedSpy(model, &ApiServicesModel::serviceSelectionChanged);

        model->updateModel(QJsonObject {
            { "user_country_code", "EE" },
            { "services", QJsonArray {} }
        });

        QCOMPARE(model->rowCount(), 1);
        QCOMPARE(selectionChangedSpy.count(), 1);
    }

    void testServicesWithoutSubscriptionPlans()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;
        model->updateModel(makeServicesResponse());

        const QModelIndex index = model->index(1, 0);

        QCOMPARE(model->data(index, ApiServicesModel::HasSubscriptionPlansRole) .toBool(), false);
    }

    void testControllerAccessors()
    {
        ApiServicesModel *model = m_coreController->m_apiServicesModel;
        model->updateModel(makeServicesResponse());
        model->setServiceIndex(0);

        ServicesCatalogUiController *controller = m_coreController->m_servicesCatalogUiController;

        QCOMPARE(controller->getSelectedServiceInfo(), QJsonObject());
        QCOMPARE(controller->getSelectedServiceType(), model->getSelectedServiceType());
        QCOMPARE(controller->getSelectedServiceProtocol(), model->getSelectedServiceProtocol());
        QCOMPARE(controller->getSelectedServiceName(), model->getSelectedServiceName());
        QCOMPARE(controller->getSelectedServiceCountries(), model->getSelectedServiceCountries());
        QCOMPARE(controller->getCountryCode(), model->getCountryCode());
        QCOMPARE(controller->getStoreEndpoint(), model->getStoreEndpoint());
        QCOMPARE(controller->getSelectedServiceData("name"), model->getSelectedServiceData("name"));
        QCOMPARE(controller->getSelectedServiceData("isPremium"), model->getSelectedServiceData("isPremium"));
    }

    void testControllerErrorSignal()
    {
        ServicesCatalogUiController *controller = m_coreController->m_servicesCatalogUiController;

        QSignalSpy errorSpy(controller, &ServicesCatalogUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());
    }
};

QTEST_MAIN(TestUiServicesModelAndController)
#include "testUiServicesModelAndController.moc"
