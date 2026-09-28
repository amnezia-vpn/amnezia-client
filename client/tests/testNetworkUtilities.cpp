#include <QtTest>

#include "core/utils/networkUtilities.h"

class TestNetworkUtilities final : public QObject {
    Q_OBJECT

private slots:
    void findsLinuxDefaultGateway();
};

void TestNetworkUtilities::findsLinuxDefaultGateway()
{
#ifdef Q_OS_LINUX
    const auto gatewayAndInterface = NetworkUtilities::getGatewayAndIface();
    QVERIFY2(!gatewayAndInterface.first.isEmpty(),
             "Linux default gateway was not found");
    QVERIFY(gatewayAndInterface.second.isValid());
#else
    QSKIP("Linux-specific regression test");
#endif
}

QTEST_GUILESS_MAIN(TestNetworkUtilities)

#include "testNetworkUtilities.moc"
