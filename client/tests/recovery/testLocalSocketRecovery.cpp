#include <QJsonDocument>
#include <QLocalServer>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "mozilla/localsocketcontroller.h"

class TestLocalSocketRecovery : public QObject
{
    Q_OBJECT

    QLocalServer m_server;
    QScopedPointer<QLocalSocket> m_peer;
    QScopedPointer<LocalSocketController> m_controller;

    static QJsonObject config()
    {
        return {{"protocol", "awg"},
                {"awg_config_data", QJsonObject{{"hostName", "192.0.2.1"},
                                                {"client_ip", "10.8.0.2"}}}};
    }

    void send(const QJsonObject& message)
    {
        m_peer->write(QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n');
        m_peer->flush();
    }

    void expectCommand(const QString& type)
    {
        QTRY_VERIFY_WITH_TIMEOUT(m_peer->canReadLine(), 1000);
        const auto message = QJsonDocument::fromJson(m_peer->readLine()).object();
        QCOMPARE(message.value("type").toString(), type);
    }

    void acceptInitialization()
    {
        QTRY_VERIFY_WITH_TIMEOUT(m_server.hasPendingConnections(), 1000);
        m_peer.reset(m_server.nextPendingConnection());
        expectCommand("status");
    }

    void ready()
    {
        acceptInitialization();
        send({{"type", "status"}, {"connected", false}});
        expectCommand("activate");
    }

private slots:
    void init()
    {
        const QString name = "amnezia-recovery-test-" + QUuid::createUuid().toString(QUuid::Id128);
        QVERIFY(m_server.listen(name));
        m_controller.reset(new LocalSocketController(name, 500));
        m_controller->initialize(nullptr, nullptr);
    }

    void cleanup()
    {
        m_controller.reset();
        m_peer.reset();
        m_server.close();
    }

    void activationWaitsForInitialization()
    {
        QSignalSpy connected(m_controller.data(), &ControllerImpl::connected);
        QSignalSpy timedOut(m_controller.data(), &ControllerImpl::connectionTimedOut);
        m_controller->activate(config());
        acceptInitialization();
        QTest::qWait(20);
        QCOMPARE(m_peer->bytesAvailable(), 0);

        send({{"type", "status"}, {"connected", false}});
        expectCommand("activate");
        send({{"type", "connected"}, {"pubkey", "test-peer"}});
        QTRY_COMPARE(connected.count(), 1);
        QTest::qWait(550);
        QCOMPARE(timedOut.count(), 0);
    }

    void cancelDuringInitialization()
    {
        QSignalSpy timedOut(m_controller.data(), &ControllerImpl::connectionTimedOut);
        m_controller->activate(config());
        acceptInitialization();
        m_controller->deactivate();
        send({{"type", "status"}, {"connected", false}});
        QTest::qWait(550);
        QCOMPARE(m_peer->bytesAvailable(), 0);
        QCOMPARE(timedOut.count(), 0);
    }

    void initializationTimeoutDiscardsPendingActivation()
    {
        QSignalSpy timedOut(m_controller.data(), &ControllerImpl::connectionTimedOut);
        m_controller->activate(config());
        acceptInitialization();
        QTRY_COMPARE_WITH_TIMEOUT(timedOut.count(), 1, 1500);
        send({{"type", "status"}, {"connected", false}});
        QTest::qWait(20);
        QCOMPARE(m_peer->bytesAvailable(), 0);
    }

    void handshakeTimeoutIgnoresLateConnected()
    {
        QSignalSpy timedOut(m_controller.data(), &ControllerImpl::connectionTimedOut);
        QSignalSpy connected(m_controller.data(), &ControllerImpl::connected);
        m_controller->activate(config());
        ready();
        QTRY_COMPARE_WITH_TIMEOUT(timedOut.count(), 1, 1500);
        expectCommand("deactivate");
        send({{"type", "connected"}, {"pubkey", "late-peer"}});
        QTest::qWait(20);
        QCOMPARE(connected.count(), 0);
    }

    void reconnectWaitsForDeactivationReply()
    {
        QSignalSpy disconnected(m_controller.data(), &ControllerImpl::disconnected);
        QSignalSpy connected(m_controller.data(), &ControllerImpl::connected);
        m_controller->activate(config());
        ready();
        m_controller->deactivate();
        expectCommand("deactivate");
        QCOMPARE(disconnected.count(), 1);

        m_controller->activate(config());
        QTest::qWait(20);
        QCOMPARE(m_peer->bytesAvailable(), 0);
        send({{"type", "disconnected"}});
        expectCommand("activate");
        QCOMPARE(disconnected.count(), 1);
        send({{"type", "connected"}, {"pubkey", "new-peer"}});
        QTRY_COMPARE(connected.count(), 1);
    }

    void cancelQueuedReconnect()
    {
        m_controller->activate(config());
        ready();
        m_controller->deactivate();
        expectCommand("deactivate");
        m_controller->activate(config());
        m_controller->deactivate();
        send({{"type", "disconnected"}});
        QTest::qWait(550);
        QCOMPARE(m_peer->bytesAvailable(), 0);
    }

    void lostServiceCanBeReconnected()
    {
        QSignalSpy failed(m_controller.data(), &ControllerImpl::backendFailure);
        m_controller->activate(config());
        ready();
        m_peer->abort();
        QTRY_COMPARE(failed.count(), 1);
        m_peer.reset();

        m_controller->activate(config());
        ready();
        QSignalSpy connected(m_controller.data(), &ControllerImpl::connected);
        send({{"type", "connected"}, {"pubkey", "recovered-peer"}});
        QTRY_COMPARE(connected.count(), 1);
    }

    void serviceBecomesAvailableDuringInitialization()
    {
        const QString name = m_server.fullServerName();
        m_controller.reset();
        m_server.close();
        m_controller.reset(new LocalSocketController(name, 2000));
        m_controller->initialize(nullptr, nullptr);
        m_controller->activate(config());
        QVERIFY(m_server.listen(name));
        ready();
        QSignalSpy connected(m_controller.data(), &ControllerImpl::connected);
        send({{"type", "connected"}, {"pubkey", "ready-peer"}});
        QTRY_COMPARE(connected.count(), 1);
    }

    void reconnectTimeoutWhileWaitingForOldTunnel()
    {
        QSignalSpy timedOut(m_controller.data(), &ControllerImpl::connectionTimedOut);
        m_controller->activate(config());
        ready();
        m_controller->deactivate();
        expectCommand("deactivate");
        m_controller->activate(config());
        QTRY_COMPARE_WITH_TIMEOUT(timedOut.count(), 1, 1500);
        send({{"type", "disconnected"}});
        QTest::qWait(20);
        QCOMPARE(m_peer->bytesAvailable(), 0);
    }

    void backendFailureEndsAttempt_data()
    {
        QTest::addColumn<QJsonObject>("failure");
        QTest::newRow("fatal") << QJsonObject{{"type", "backendFailure"}, {"errorCode", 1}};
        QTest::newRow("unknown-code") << QJsonObject{{"type", "backendFailure"}, {"errorCode", 999}};
        QTest::newRow("missing-code") << QJsonObject{{"type", "backendFailure"}};
    }

    void backendFailureEndsAttempt()
    {
        QFETCH(QJsonObject, failure);
        QSignalSpy failed(m_controller.data(), &ControllerImpl::backendFailure);
        QSignalSpy timedOut(m_controller.data(), &ControllerImpl::connectionTimedOut);
        m_controller->activate(config());
        ready();
        send(failure);
        QTRY_COMPARE(failed.count(), 1);
        expectCommand("deactivate");
        QTest::qWait(550);
        QCOMPARE(timedOut.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestLocalSocketRecovery)
#include "testLocalSocketRecovery.moc"
