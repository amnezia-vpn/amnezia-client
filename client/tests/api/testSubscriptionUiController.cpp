#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "utils/testCoreController.h"

#include "secureQSettings.h"
#include "ui/controllers/api/subscriptionUiController.h"
#include "vpnConnection.h"

using namespace amnezia;

class TestSubscriptionUiController : public QObject
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
        QVERIFY(m_coreController->m_subscriptionUiController != nullptr);
    }

    
    void testInitialState()
    {
        QCOMPARE(m_coreController->m_subscriptionUiController->property("qrCodesCount").toInt(), 0);
        QVERIFY(m_coreController->m_subscriptionUiController->property("qrCodes").toStringList().isEmpty() || controller()->property("qrCodes").toList().isEmpty());
        QVERIFY(m_coreController->m_subscriptionUiController->property("vpnKey").toString().isEmpty());
        QVERIFY(!m_coreController->m_subscriptionUiController->isCaptchaAwaitingUser());
    }


    void testExportVpnKeyWithEmptyFileName()
    {
        QSignalSpy errorSpy(m_coreController->m_subscriptionUiController, &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());

        const bool result = m_coreController->m_subscriptionUiController->exportVpnKey(QStringLiteral("unknown-server"), QString());

        QVERIFY(!result);
        QCOMPARE(errorSpy.count(), 1);

        // Error rise before request to server: filename is required
        QVERIFY(m_coreController->m_subscriptionUiController->property("vpnKey").toString().isEmpty());
        QCOMPARE(m_coreController->m_subscriptionUiController->property("qrCodesCount").toInt(), 0);
    }

    void testGetRenewalLinkWithEmptyServerId()
    {
        QSignalSpy errorSpy(m_coreController->m_subscriptionUiController, &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());

        m_coreController->m_subscriptionUiController->getRenewalLink(QString());

        QVERIFY(errorSpy.count() == 1, "Empty serverId should emit InternalError, not execute request");
    }

    void testCurrentActivePlanInfoWithoutActivePurchase()
    {
        const QVariantMap info = m_coreController->m_subscriptionUiController->currentActivePlanInfo();

        QVERIFY(info.contains(QStringLiteral("hasActivePlan")));
        QCOMPARE(info.value(QStringLiteral("hasActivePlan")).toBool(), false);
    }

    void testErrorSignalIsAvailable()
    {
        QSignalSpy errorSpy(m_coreController->m_subscriptionUiController, &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());
        QCOMPARE(errorSpy.count(), 0);
    }
};

QTEST_MAIN(TestSubscriptionUiController)
#include "testSubscriptionUiController.moc"