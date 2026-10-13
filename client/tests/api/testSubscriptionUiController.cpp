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
    TestCoreController *m_coreController = nullptr;
    SecureQSettings *m_settings = nullptr;

    SubscriptionUiController *controller() const
    {
        return m_coreController->m_subscriptionUiController;
    }

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
        QVERIFY(controller() != nullptr);
    }

    
    void testInitialState()
    {
        QCOMPARE(controller()->property("qrCodesCount").toInt(), 0);
        QVERIFY(controller()->property("qrCodes").toStringList().isEmpty() || controller()->property("qrCodes").toList().isEmpty());
        QVERIFY(controller()->property("vpnKey").toString().isEmpty());
        QVERIFY(!controller()->isCaptchaAwaitingUser());
    }


    void testExportVpnKeyWithEmptyFileName()
    {
        QSignalSpy errorSpy(controller(), &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());

        const bool result = controller()->exportVpnKey(QStringLiteral("unknown-server"), QString());

        QVERIFY(!result);
        QCOMPARE(errorSpy.count(), 1);

        // Error rise before request to server: filename is required
        QVERIFY(controller()->property("vpnKey").toString().isEmpty());
        QCOMPARE(controller()->property("qrCodesCount").toInt(), 0);
    }

    void testGetRenewalLinkWithEmptyServerId()
    {
        QSignalSpy errorSpy(controller(), &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());

        controller()->getRenewalLink(QString());

        QVERIFY(errorSpy.count() == 1, "Empty serverId should emit InternalError, not execute request");
    }

    void testCurrentActivePlanInfoWithoutActivePurchase()
    {
        const QVariantMap info = controller()->currentActivePlanInfo();

        QVERIFY(info.contains(QStringLiteral("hasActivePlan")));
        QCOMPARE(info.value(QStringLiteral("hasActivePlan")).toBool(), false);
    }

    void testErrorSignalIsAvailable()
    {
        QSignalSpy errorSpy(controller(), &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());
        QCOMPARE(errorSpy.count(), 0);
    }
};

QTEST_MAIN(TestSubscriptionUiController)
#include "testSubscriptionUiController.moc"