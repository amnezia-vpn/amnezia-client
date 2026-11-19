#include <QSignalSpy>
#include <QTest>

#include "core/controllers/splitTunnelingUpdateController.h"

class TestSplitTunnelingUnsupportedBackend : public QObject
{
    Q_OBJECT

private slots:
    void unsupportedPlatformCannotStartALiveUpdate()
    {
        auto *backend = SplitTunnelingUpdateBackend::create(this);
        QVERIFY(!backend);
        SplitTunnelingUpdateController controller(backend);
        QSignalSpy applied(&controller, &SplitTunnelingUpdateController::configurationApplied);
        controller.startSession({ });
        QVERIFY(!controller.isAvailable());
        controller.requestUpdate({ });
        QCOMPARE(applied.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestSplitTunnelingUnsupportedBackend)
#include "testSplitTunnelingUnsupportedBackend.moc"
