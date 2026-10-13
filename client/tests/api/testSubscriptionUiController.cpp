
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "utils/testCoreController.h"
#include "utils/testUtils.h"

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
    SubscriptionUiController *m_subscriptionUiController = nullptr;

private slots:
    void initTestCase()
    {
        const QString testOrg = "AmneziaVPN-Test-" + QUuid::createUuid().toString();
        m_settings = new SecureQSettings(testOrg, "amnezia-client", nullptr, false);

        const auto vpnConnection = QSharedPointer<VpnConnection>::create(nullptr, nullptr);

        m_coreController = new TestCoreController(vpnConnection, m_settings, nullptr, this);

        m_subscriptionUiController = m_coreController->findChild<SubscriptionUiController *>();

        QVERIFY2(m_subscriptionUiController != nullptr, "SubscriptionUiController should be created by CoreController");
    }

    void cleanupTestCase()
    {
        m_settings->clearSettings();
        delete m_coreController;
        delete m_settings;

        m_subscriptionUiController = nullptr;
    }

    void init()
    {
        m_settings->clearSettings();
        QVERIFY2(m_subscriptionUiController != nullptr, "SubscriptionUiController should be available");
    }

    void testInitialState()
    {
        QCOMPARE(m_subscriptionUiController->property("qrCodesCount").toInt(), 0);
        QCOMPARE(m_subscriptionUiController->property("qrCodes").toList().size(), 0);
        QVERIFY(m_subscriptionUiController->property("vpnKey").toString().isEmpty());

        QVERIFY(!m_subscriptionUiController->isCaptchaAwaitingUser());
    }

    void testExportVpnKeyWithEmptyFileName()
    {
        QSignalSpy errorSpy(m_subscriptionUiController, &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());

        const bool result = m_subscriptionUiController->exportVpnKey(QStringLiteral("unknown-server"), QString());

        QVERIFY2(!result, "Export should fail when the file name is empty");
        QCOMPARE(errorSpy.count(), 1);

        QCOMPARE(m_subscriptionUiController->property("vpnKey").toString(), QString());
        QCOMPARE(m_subscriptionUiController->property("qrCodesCount").toInt(), 0);
    }

    void testGetRenewalLinkWithEmptyServerId()
    {
        QSignalSpy errorSpy(m_subscriptionUiController, &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());

        m_subscriptionUiController->getRenewalLink(QString());

        QVERIFY2(errorSpy.count() == 1, "An empty server ID should emit errorOccurred");
    }

    void testCurrentActivePlanInfoWithoutActivePurchase()
    {
        const QVariantMap info = m_subscriptionUiController->currentActivePlanInfo();

        QVERIFY(info.contains(QStringLiteral("hasActivePlan")));
        QVERIFY2(!info.value(QStringLiteral("hasActivePlan")).toBool(), "No active store purchase should be reported");
    }

    void testErrorSignalIsAvailable()
    {
        QSignalSpy errorSpy(m_subscriptionUiController, &SubscriptionUiController::errorOccurred);

        QVERIFY(errorSpy.isValid());
        QCOMPARE(errorSpy.count(), 0);
    }
};

QTEST_MAIN(TestSubscriptionUiController)
#include "testSubscriptionUiController.moc"