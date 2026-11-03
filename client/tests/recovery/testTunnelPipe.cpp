#include <QElapsedTimer>
#include <QLocalServer>
#include <QTest>
#include <QTimer>
#include <QScopeGuard>
#include <QUuid>
#include <chrono>
#include <future>

#include "platforms/windows/daemon/windowstunnelpipe.h"

using namespace std::chrono_literals;

class TestTunnelPipe : public QObject
{
    Q_OBJECT

    QLocalServer m_server;
    QScopedPointer<QLocalSocket> m_peer;

    std::future<QString> request(const QString& command = "get=1", int timeoutMsec = 500)
    {
        return std::async(std::launch::async, [name = m_server.fullServerName(), command, timeoutMsec] {
            return WindowsTunnelPipe::request(name, command, timeoutMsec);
        });
    }

    void acceptRequest()
    {
        QTRY_VERIFY_WITH_TIMEOUT(m_server.hasPendingConnections(), 1000);
        m_peer.reset(m_server.nextPendingConnection());
    }

private slots:
    void init()
    {
        QVERIFY(m_server.listen("amnezia-pipe-test-" + QUuid::createUuid().toString(QUuid::Id128)));
    }

    void cleanup()
    {
        m_peer.reset();
        m_server.close();
    }

    void fragmentedReply()
    {
        auto result = request();
        acceptRequest();
        QTRY_COMPARE(m_peer->bytesAvailable(), 7);
        QCOMPARE(m_peer->readAll(), QByteArray("get=1\n\n"));
        m_peer->write("public_key=test\nerrno=0\n");
        m_peer->flush();
        QTest::qWait(20);
        QVERIFY(result.wait_for(0ms) != std::future_status::ready);
        m_peer->write("\n");
        m_peer->flush();
        QTRY_VERIFY(result.wait_for(0ms) == std::future_status::ready);
        QCOMPARE(result.get(), QString("public_key=test\nerrno=0"));
    }

    void incompleteReply_data()
    {
        QTest::addColumn<QByteArray>("reply");
        QTest::newRow("silent-service") << QByteArray();
        QTest::newRow("missing-terminator") << QByteArray("errno=0\n");
    }

    void incompleteReply()
    {
        QFETCH(QByteArray, reply);
        QElapsedTimer elapsed;
        elapsed.start();
        auto result = request();
        acceptRequest();
        if (!reply.isEmpty()) {
            m_peer->write(reply);
            m_peer->flush();
        }
        QTRY_VERIFY_WITH_TIMEOUT(result.wait_for(0ms) == std::future_status::ready, 2000);
        QVERIFY(result.get().isEmpty());
        QVERIFY(elapsed.elapsed() < 2000);
    }

    void trickleReplyDoesNotResetDeadline()
    {
        QElapsedTimer elapsed;
        elapsed.start();
        auto result = request();
        acceptRequest();
        QTimer trickle;
        connect(&trickle, &QTimer::timeout, this, [this] {
            m_peer->write("x");
            m_peer->flush();
        });
        trickle.start(30);
        QTRY_VERIFY_WITH_TIMEOUT(result.wait_for(0ms) == std::future_status::ready, 2000);
        QVERIFY(result.get().isEmpty());
        QVERIFY(elapsed.elapsed() < 2000);
    }

    void peerClosesBeforeReply()
    {
        auto result = request();
        acceptRequest();
        m_peer->abort();
        QTRY_VERIFY_WITH_TIMEOUT(result.wait_for(0ms) == std::future_status::ready, 2000);
        QVERIFY(result.get().isEmpty());
    }

    void blockedWriteTimesOut()
    {
        auto result = request(QString(8 * 1024 * 1024, 'x'));
        acceptRequest();
        m_peer->setReadBufferSize(1);
        QTRY_VERIFY_WITH_TIMEOUT(result.wait_for(0ms) == std::future_status::ready, 2000);
        QVERIFY(result.get().isEmpty());
    }

    void missingService()
    {
        const QString name = m_server.fullServerName();
        m_server.close();
        QVERIFY(WindowsTunnelPipe::request(name, "get=1", 100).isEmpty());
    }

#ifdef Q_OS_WIN
    void busyPipeHonorsDeadline()
    {
        const QString name = "\\\\.\\pipe\\amnezia-busy-test-" + QUuid::createUuid().toString(QUuid::Id128);
        const HANDLE pipe = CreateNamedPipeW(reinterpret_cast<LPCWSTR>(name.utf16()),
                                             PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                             PIPE_TYPE_BYTE | PIPE_WAIT, 1, 1024, 1024, 0, nullptr);
        QVERIFY(pipe != INVALID_HANDLE_VALUE);
        const auto cleanup = qScopeGuard([pipe] { CloseHandle(pipe); });
        QLocalSocket occupied;
        occupied.connectToServer(name);
        QVERIFY(occupied.waitForConnected(1000));

        QElapsedTimer elapsed;
        elapsed.start();
        QVERIFY(WindowsTunnelPipe::request(name, "get=1", 100).isEmpty());
        QVERIFY(elapsed.elapsed() < 1000);
    }
#endif
};

QTEST_GUILESS_MAIN(TestTunnelPipe)
#include "testTunnelPipe.moc"
