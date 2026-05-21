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

class TestUiApiServicesModelAndController : public QObject
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
        QSignalSpy errorOccurredSpy(m_coreController->m_servicesCatalogUiController, &ServicesCatalogUiController::errorOccurred);

        /* TODO:
        m_coreController->m_servicesCatalogUiController->fillAvailableServices();
        QVERIFY(errorOccurredSpy.count() == 0, "errorOccurred signal should not be emitted");

        QModelIndex serviceModelIndex = m_coreController->m_apiServicesModel->index(0, 0);
        QVERIFY2(serviceModelIndex.isValid(), "Service model index should be valid");

        auto serviceName = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::NameRole);
        QCOMPARE(serviceName, );

        auto serviceCardDescription = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::CardDescriptionRole);
        QCOMPARE(serviceCardDescription, );

        auto isServiceAvailable = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::IsServiceAvailableRole);
        QCOMPARE(isServiceAvailable, );

        auto serviceSpeed = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::SpeedRole);
        QCOMPARE(serviceSpeed, );

        auto serviceTimeLimit = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::TimeLimitRole);
        QCOMPARE(serviceTimeLimit, );

        auto serviceRegion = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::RegionRole);
        QCOMPARE(serviceRegion, );

        auto serviceFeatures = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::FeaturesRole);
        QCOMPARE(serviceFeatures, );

        auto servicePrice = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::PriceRole);
        QCOMPARE(servicePrice, );

        auto serviceEndDate = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::EndDateRole);
        QCOMPARE(serviceEndDate, );

        auto serviceOrder = m_coreController->m_apiServicesModel->data(serviceModelIndex, ApiServicesModel::OrderRole);
        QCOMPARE(serviceOrder, );
        */
    }
};

QTEST_MAIN(TestUiApiServicesModelAndController)
#include "testUiApiServicesModelAndController.moc"
