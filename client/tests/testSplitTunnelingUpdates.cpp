#include <QJsonArray>
#include <QSignalSpy>
#include <QTest>

#include "core/controllers/splitTunnelingUpdateController.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"

using namespace amnezia;

class TestSplitTunnelingBackend : public SplitTunnelingUpdateBackend
{
public:
    using SplitTunnelingUpdateBackend::SplitTunnelingUpdateBackend;
    struct Request
    {
        quint64 id;
        QJsonObject configuration;
    };
    QList<Request> requests;
    bool supported = true;

    bool supportsConfiguration(const QJsonObject &configuration) const override
    {
        return supported && SplitTunnelingUpdateBackend::supportsConfiguration(configuration);
    }
    void apply(quint64 requestId, const QJsonObject &configuration) override
    {
        requests.append({ requestId, configuration });
    }
    void complete(quint64 requestId, SplitTunnelingUpdateResult result)
    {
        emit finished(requestId, result);
    }
};

class TestSplitTunnelingUpdates : public QObject
{
    Q_OBJECT

    static QJsonObject configuration(const QStringList &sites, Proto protocol = Proto::Awg)
    {
        const auto serializeClientConfig = [](auto config) {
            config.serverPublicKey = QStringLiteral("test-peer");
            config.allowedIps = QStringList { "0.0.0.0/0", "::/0" };
            return config.toJson();
        };
        const auto clientConfig = protocol == Proto::Awg ? serializeClientConfig(AwgClientConfig { })
                                                         : serializeClientConfig(WireGuardClientConfig { });
        return { { configKey::vpnProto, ProtocolUtils::protoToString(protocol) },
                 { ProtocolUtils::key_proto_config_data(protocol), clientConfig },
                 { configKey::splitTunnelSessionId, "test-session" },
                 { configKey::splitTunnelType, 1 },
                 { configKey::splitTunnelSites, QJsonArray::fromStringList(sites) } };
    }

private slots:
    void unavailableBackendKeepsLiveUpdatesDisabled()
    {
        SplitTunnelingUpdateController absent(nullptr);
        absent.startSession(configuration({ }));
        QVERIFY(!absent.isAvailable());
        absent.requestUpdate(configuration({ "192.0.2.1" }));

        TestSplitTunnelingBackend backend;
        backend.supported = false;
        SplitTunnelingUpdateController controller(&backend);
        controller.startSession(configuration({ }));
        QVERIFY(!controller.isAvailable());
        controller.requestUpdate(configuration({ "192.0.2.1" }));
        QVERIFY(backend.requests.isEmpty());
    }

    void configurationSupport_data()
    {
        QTest::addColumn<QJsonObject>("config");
        QTest::addColumn<bool>("supported");
        auto config = configuration({ "192.0.2.1" });
        QTest::newRow("awg-inclusion") << config << true;
        config.insert(configKey::splitTunnelType, 2);
        QTest::newRow("awg-exclusion") << config << true;
        config = configuration({ "192.0.2.1" }, Proto::WireGuard);
        QTest::newRow("wireguard") << config << true;
        config = configuration({ "192.0.2.1" });
        config.insert(configKey::vpnProto, "AmneziaWG");
        QTest::newRow("awg-display-name") << config << false;
        config = configuration({ "192.0.2.1" }, Proto::WireGuard);
        config.insert(configKey::vpnProto, "WireGuard");
        QTest::newRow("wireguard-display-name") << config << false;
        config.insert(configKey::vpnProto, ProtocolUtils::protoToString(Proto::OpenVpn));
        QTest::newRow("other-protocol") << config << false;
        config = configuration({ });
        auto native = config.value(ProtocolUtils::key_proto_config_data(Proto::Awg)).toObject();
        native.insert(configKey::allowedIps, QJsonArray { "192.0.2.0/24" });
        config.insert(ProtocolUtils::key_proto_config_data(Proto::Awg), native);
        QTest::newRow("native-custom-routes") << config << false;
        config = configuration({ });
        config.remove(configKey::splitTunnelSessionId);
        QTest::newRow("restored-without-session") << config << false;
        config = configuration({ "example.com" });
        QTest::newRow("unresolved-domain") << config << false;
        config = configuration({ "192.0.2.1/99" });
        QTest::newRow("invalid-prefix") << config << false;
    }

    void configurationSupport()
    {
        QFETCH(QJsonObject, config);
        QFETCH(bool, supported);
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        controller.startSession(config);
        QCOMPARE(controller.isAvailable(), supported);
    }

    void unchangedConfigurationDoesNotApply()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        const auto initial = configuration({ "192.0.2.1" });
        controller.startSession(initial);
        controller.requestUpdate(initial);
        QVERIFY(backend.requests.isEmpty());
    }

    void concurrentChangesApplyTheLatestCompleteList()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy applied(&controller, &SplitTunnelingUpdateController::configurationApplied);
        controller.startSession(configuration({ }));
        controller.requestUpdate(configuration({ "192.0.2.1" }));
        controller.requestUpdate(configuration({ "192.0.2.1", "192.0.2.2" }));
        const auto latest = configuration({ "192.0.2.1", "192.0.2.2", "198.51.100.0/24" });
        controller.requestUpdate(latest);
        QCOMPARE(backend.requests.size(), 1);
        backend.complete(backend.requests.first().id, SplitTunnelingUpdateResult::Applied);
        QTRY_COMPARE(backend.requests.size(), 2);
        QCOMPARE(backend.requests.last().configuration, latest);
        backend.complete(backend.requests.last().id, SplitTunnelingUpdateResult::Applied);
        QTRY_COMPARE(applied.count(), 2);
        QCOMPARE(applied.last().first().toJsonObject(), latest);
        controller.requestUpdate(latest);
        QCOMPARE(backend.requests.size(), 2);
    }

    void unknownStateDisablesLiveChangesUntilNewSession()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy failed(&controller, &SplitTunnelingUpdateController::updateFailed);
        QSignalSpy applied(&controller, &SplitTunnelingUpdateController::configurationApplied);
        controller.startSession(configuration({ }));
        const auto updated = configuration({ "192.0.2.1" });
        controller.requestUpdate(updated);
        const quint64 firstRequest = backend.requests.first().id;
        controller.requestUpdate(configuration({ "192.0.2.1", "192.0.2.2" }));
        backend.complete(firstRequest, SplitTunnelingUpdateResult::StateUnknown);
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(qvariant_cast<SplitTunnelingUpdateResult>(failed.first().first()),
                 SplitTunnelingUpdateResult::StateUnknown);
        QVERIFY(!controller.isAvailable());
        controller.requestUpdate(updated);
        QCOMPARE(backend.requests.size(), 1);
        QCOMPARE(applied.count(), 0);
        controller.startSession(configuration({ }));
        QVERIFY(controller.isAvailable());
        controller.requestUpdate(updated);
        QCOMPARE(backend.requests.size(), 2);
        QVERIFY(backend.requests.last().id != firstRequest);
    }

    void confirmedRejectionCanBeRetriedWithoutAnAutomaticLoop()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy failed(&controller, &SplitTunnelingUpdateController::updateFailed);
        QSignalSpy applied(&controller, &SplitTunnelingUpdateController::configurationApplied);
        controller.startSession(configuration({ }));
        const auto updated = configuration({ "192.0.2.1" });
        controller.requestUpdate(updated);
        const auto rejectedRequest = backend.requests.first().id;
        backend.complete(rejectedRequest, SplitTunnelingUpdateResult::NotApplied);
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(qvariant_cast<SplitTunnelingUpdateResult>(failed.first().first()),
                 SplitTunnelingUpdateResult::NotApplied);
        QVERIFY(controller.isAvailable());
        QCOMPARE(backend.requests.size(), 1);
        QCOMPARE(applied.count(), 0);
        controller.requestUpdate(updated);
        QCOMPARE(backend.requests.size(), 2);
        QVERIFY(backend.requests.last().id != rejectedRequest);
        backend.complete(backend.requests.last().id, SplitTunnelingUpdateResult::Applied);
        QTRY_COMPARE(applied.count(), 1);
    }

    void confirmedRejectionDoesNotDiscardNewerChanges()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        controller.startSession(configuration({ }));
        controller.requestUpdate(configuration({ "192.0.2.1" }));
        const auto latest = configuration({ "192.0.2.1", "192.0.2.2" });
        controller.requestUpdate(latest);
        backend.complete(backend.requests.first().id, SplitTunnelingUpdateResult::NotApplied);
        QTRY_COMPARE(backend.requests.size(), 2);
        QCOMPARE(backend.requests.last().configuration, latest);
        QVERIFY(controller.isAvailable());
    }

    void transportValuesAreDecodedConservatively()
    {
        QCOMPARE(SplitTunnelingUpdateResult { }, SplitTunnelingUpdateResult::StateUnknown);
        for (const auto result : { SplitTunnelingUpdateResult::Applied, SplitTunnelingUpdateResult::NotApplied,
                                   SplitTunnelingUpdateResult::StateUnknown }) {
            QCOMPARE(splitTunnelingUpdateResultFromInt(static_cast<int>(result)), result);
        }
        QCOMPARE(splitTunnelingUpdateResultFromInt(-1), SplitTunnelingUpdateResult::StateUnknown);
        QCOMPARE(splitTunnelingUpdateResultFromInt(99), SplitTunnelingUpdateResult::StateUnknown);
    }

    void firstSiteInAnEmptyListCanBeApplied_data()
    {
        QTest::addColumn<int>("mode");
        QTest::newRow("include") << 1;
        QTest::newRow("exclude") << 2;
    }

    void firstSiteInAnEmptyListCanBeApplied()
    {
        QFETCH(int, mode);
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        auto empty = configuration({ });
        empty.insert(configKey::splitTunnelType, 0);
        controller.startSession(empty);
        auto updated = configuration({ "192.0.2.1" });
        updated.insert(configKey::splitTunnelType, mode);
        controller.requestUpdate(updated);
        QCOMPARE(backend.requests.size(), 1);
        QCOMPARE(backend.requests.first().configuration, updated);
    }

    void queuedUpdateCannotRemovePendingAdditions()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy failed(&controller, &SplitTunnelingUpdateController::updateFailed);
        controller.startSession(configuration({ }));
        controller.requestUpdate(configuration({ "192.0.2.1" }));
        controller.requestUpdate(configuration({ "192.0.2.2" }));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(backend.requests.size(), 1);
    }

    void disconnectCancelsPendingChangesAndLateCallbacks()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy applied(&controller, &SplitTunnelingUpdateController::configurationApplied);
        QSignalSpy failed(&controller, &SplitTunnelingUpdateController::updateFailed);
        controller.startSession(configuration({ }));
        controller.requestUpdate(configuration({ "192.0.2.1" }));
        const auto requestId = backend.requests.first().id;
        controller.requestUpdate(configuration({ "192.0.2.1", "192.0.2.2" }));
        controller.cancel();
        backend.complete(requestId, SplitTunnelingUpdateResult::Applied);
        QCoreApplication::processEvents();
        controller.requestUpdate(configuration({ "198.51.100.1" }));
        QCOMPARE(backend.requests.size(), 1);
        QCOMPARE(applied.count(), 0);
        QCOMPARE(failed.count(), 0);
        QVERIFY(!controller.isAvailable());
    }

    void oldSessionCannotCompleteNewSessionUpdate()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy applied(&controller, &SplitTunnelingUpdateController::configurationApplied);
        controller.startSession(configuration({ }));
        controller.requestUpdate(configuration({ "192.0.2.1" }));
        const auto oldRequest = backend.requests.first().id;
        controller.startSession(configuration({ }));
        const auto current = configuration({ "198.51.100.1" });
        controller.requestUpdate(current);
        backend.complete(oldRequest, SplitTunnelingUpdateResult::Applied);
        QCoreApplication::processEvents();
        QCOMPARE(applied.count(), 0);
        backend.complete(backend.requests.last().id, SplitTunnelingUpdateResult::Applied);
        QTRY_COMPARE(applied.count(), 1);
        QCOMPARE(applied.first().first().toJsonObject(), current);
    }

    void invalidUpdateDoesNotReachBackend()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy failed(&controller, &SplitTunnelingUpdateController::updateFailed);
        controller.startSession(configuration({ }));
        controller.requestUpdate(configuration({ "example.com" }));
        QCOMPARE(failed.count(), 1);
        QVERIFY(backend.requests.isEmpty());
    }

    void unsupportedChangesDoNotReachBackend_data()
    {
        QTest::addColumn<QJsonObject>("updated");
        auto updated = configuration({ "192.0.2.1", "192.0.2.2" });
        updated.insert(configKey::splitTunnelSessionId, "another-session");
        QTest::newRow("session-change") << updated;
        updated = configuration({ "192.0.2.1", "192.0.2.2" });
        updated.insert(configKey::splitTunnelType, 2);
        QTest::newRow("mode-change") << updated;
        updated = configuration({ "192.0.2.2" });
        QTest::newRow("removal") << updated;
        updated = configuration({ "192.0.2.1", "192.0.2.2" });
        updated.insert(configKey::splitTunnelApps, QJsonArray { "another-app" });
        QTest::newRow("app-policy-change") << updated;
    }

    void unsupportedChangesDoNotReachBackend()
    {
        QFETCH(QJsonObject, updated);
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy failed(&controller, &SplitTunnelingUpdateController::updateFailed);
        controller.startSession(configuration({ "192.0.2.1" }));
        controller.requestUpdate(updated);
        QCOMPARE(failed.count(), 1);
        QVERIFY(backend.requests.isEmpty());
    }

    void missingCompletionDisablesFurtherUpdates()
    {
        TestSplitTunnelingBackend backend;
        SplitTunnelingUpdateController controller(&backend);
        QSignalSpy failed(&controller, &SplitTunnelingUpdateController::updateFailed);
        QSignalSpy applied(&controller, &SplitTunnelingUpdateController::configurationApplied);
        controller.startSession(configuration({ }));
        controller.findChild<QTimer *>()->setInterval(1);
        controller.requestUpdate(configuration({ "192.0.2.1" }));
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(qvariant_cast<SplitTunnelingUpdateResult>(failed.first().first()),
                 SplitTunnelingUpdateResult::StateUnknown);
        QVERIFY(!controller.isAvailable());
        controller.requestUpdate(configuration({ "192.0.2.1", "192.0.2.2" }));
        backend.complete(backend.requests.first().id, SplitTunnelingUpdateResult::Applied);
        QCoreApplication::processEvents();
        QCOMPARE(backend.requests.size(), 1);
        QCOMPARE(applied.count(), 0);
    }

    void destroyedBackendCancelsTheSession()
    {
        auto *backend = new TestSplitTunnelingBackend;
        SplitTunnelingUpdateController controller(backend);
        QSignalSpy failed(&controller, &SplitTunnelingUpdateController::updateFailed);
        controller.startSession(configuration({ }));
        controller.requestUpdate(configuration({ "192.0.2.1" }));
        delete backend;
        QVERIFY(!controller.isAvailable());
        QCOMPARE(failed.count(), 1);
    }
};

QTEST_GUILESS_MAIN(TestSplitTunnelingUpdates)
#include "testSplitTunnelingUpdates.moc"
