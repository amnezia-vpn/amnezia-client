#include "core/protocols/tun2SocksStderrReader.h"
#include "rep_ipc_process_interface_replica.h"
#include "rep_ipc_process_interface_source.h"

#include <QEventLoop>
#include <QRemoteObjectHost>
#include <QSignalSpy>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QUuid>

#include <atomic>
#include <memory>

namespace
{
    class ProcessSource final : public IpcProcessInterfaceSimpleSource
    {
        Q_OBJECT

    public:
        explicit ProcessSource(int extraNotifications) : m_extraNotifications(extraNotifications)
        {
        }

        void start() override
        {
        }
        void terminate() override
        {
        }
        void kill() override
        {
        }
        void close() override
        {
        }
        void setArguments(const QStringList &) override
        {
        }
        void setInputChannelMode(QProcess::InputChannelMode) override
        {
        }
        void setNativeArguments(const QString &) override
        {
        }
        void setProcessChannelMode(QProcess::ProcessChannelMode) override
        {
        }
        void setProgram(int) override
        {
        }
        void setWorkingDirectory(const QString &) override
        {
        }
        QByteArray readAll() override
        {
            return { };
        }
        QByteArray readAllStandardOutput() override
        {
            return "barrier";
        }
        bool waitForStarted() override
        {
            return false;
        }
        bool waitForStarted(int) override
        {
            return false;
        }
        bool waitForFinished() override
        {
            return false;
        }
        bool waitForFinished(int) override
        {
            return false;
        }

        QByteArray readAllStandardError() override
        {
            switch (++readCount) {
            case 1: {
                QEventLoop waitForRelease;
                connect(this, &ProcessSource::releaseReply, &waitForRelease, &QEventLoop::quit);
                QTimer::singleShot(5000, &waitForRelease, &QEventLoop::quit);
                for (int i = 0; i < m_extraNotifications; ++i) {
                    emit readyReadStandardError();
                }
                waitForRelease.exec();
                return "first chunk";
            }
            case 2: return "second chunk";
            default: return { };
            }
        }

        std::atomic<int> readCount { 0 };

    signals:
        void releaseReply();

    private:
        const int m_extraNotifications;
    };

    class RemoteProcess
    {
    public:
        explicit RemoteProcess(int extraNotifications) : source(new ProcessSource(extraNotifications))
        {
            const QUrl url("local:amnezia-stderr-test-" + QUuid::createUuid().toString(QUuid::Id128));
            source->moveToThread(&m_thread);
            QObject::connect(&m_thread, &QThread::finished, source, &QObject::deleteLater);
            m_thread.start();

            bool listening = false;
            QMetaObject::invokeMethod(
                    source,
                    [&]() {
                        auto *host = new QRemoteObjectHost(url, source);
                        listening = host->enableRemoting(source);
                    },
                    Qt::BlockingQueuedConnection);
            if (listening && m_node.connectToNode(url)) {
                replica.reset(m_node.acquire<IpcProcessInterfaceReplica>());
            }
        }

        ~RemoteProcess()
        {
            releaseReply();
            m_thread.quit();
            m_thread.wait();
        }

        void notify()
        {
            QMetaObject::invokeMethod(source, &ProcessSource::readyReadStandardError, Qt::QueuedConnection);
        }

        void releaseReply()
        {
            QMetaObject::invokeMethod(source, &ProcessSource::releaseReply, Qt::QueuedConnection);
        }

        ProcessSource *const source;

    private:
        QThread m_thread;
        QRemoteObjectNode m_node;

    public:
        std::unique_ptr<IpcProcessInterfaceReplica> replica;
    };
}

class TestTun2SocksStderrReader : public QObject
{
    Q_OBJECT

private slots:
    void coalescesNotificationsWithoutEnteringAnotherEventLoop()
    {
        constexpr int extraNotifications = 3;
        RemoteProcess process(extraNotifications);
        QVERIFY(process.replica && process.replica->waitForSource(5000));

        Tun2SocksStderrReader reader;
        reader.setProcess(process.replica.get());
        QSignalSpy output(&reader, &Tun2SocksStderrReader::outputReady);

        const int initialLoopLevel = QThread::currentThread()->loopLevel();
        int loopLevelDuringRead = -1;
        int notifications = 0;
        connect(process.replica.get(), &IpcProcessInterfaceReplica::readyReadStandardError, &reader, [&]() {
            if (++notifications == 1 + extraNotifications) {
                QMetaObject::invokeMethod(
                        &reader,
                        [&]() {
                            loopLevelDuringRead = QThread::currentThread()->loopLevel();
                            process.releaseReply();
                        },
                        Qt::QueuedConnection);
            }
        });
        process.notify();

        QTRY_VERIFY(loopLevelDuringRead >= 0);
        QCOMPARE(loopLevelDuringRead, initialLoopLevel);
        QTRY_COMPARE(output.count(), 2);
        QCOMPARE(output.at(0).at(0).toByteArray(), QByteArray("first chunk"));
        QCOMPARE(output.at(1).at(0).toByteArray(), QByteArray("second chunk"));
        QCOMPARE(process.source->readCount.load(), 2);
    }

    void discardsReplyAfterProcessIsCleared()
    {
        RemoteProcess process(0);
        QVERIFY(process.replica && process.replica->waitForSource(5000));

        Tun2SocksStderrReader reader;
        reader.setProcess(process.replica.get());
        QSignalSpy output(&reader, &Tun2SocksStderrReader::outputReady);
        process.notify();

        QTRY_COMPARE(process.source->readCount.load(), 1);
        reader.setProcess(nullptr);
        process.releaseReply();

        auto barrier = process.replica->readAllStandardOutput();
        QVERIFY(barrier.waitForFinished(5000));
        QCOMPARE(barrier.returnValue(), QByteArray("barrier"));
        QCoreApplication::processEvents();
        QCOMPARE(output.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestTun2SocksStderrReader)
#include "testTun2SocksStderrReader.moc"
