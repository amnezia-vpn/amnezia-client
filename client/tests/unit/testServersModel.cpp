#include <QSignalSpy>
#include <QTest>
#include <QAbstractItemModelTester>

#include "ui/models/serversModel.h"
#include "core/models/serverDescription.h"

using namespace amnezia;

class TestServersModel : public QObject
{
    Q_OBJECT

private:
    ServersModel m_model;

    static ServerDescription createServer(
        const QString &id,
        const QString &name,
        const QString &hostName)
    {
        ServerDescription server;
        server.serverId = id;
        server.serverName = name;
        server.hostName = hostName;
        server.baseDescription = QStringLiteral("VPN | ");
        server.hasWriteAccess = true;
        server.hasInstalledVpnContainers = true;
        return server;
    }

private slots:
    void init()
    {
        m_model.updateModel({}, QString());
    }

    void testInitialState()
    {
        QCOMPARE(m_model.rowCount(), 0);
        QCOMPARE(m_model.data(-1, ServersModel::NameRole), QVariant());
        QCOMPARE(m_model.data(0, ServersModel::NameRole), QVariant());
    }

    void testUpdateModel()
    {
        const QVector<ServerDescription> servers = {
            createServer("server-1", "Server 1", "one.example.com"),
            createServer("server-2", "Server 2", "two.example.com")
        };

        QSignalSpy resetSpy( &m_model, &QAbstractItemModel::modelReset);

        m_model.updateModel(servers, "server-2");

        QCOMPARE(m_model.rowCount(), 2);
        QCOMPARE(resetSpy.count(), 1);

        QCOMPARE(m_model.data(0, ServersModel::NameRole).toString(), QStringLiteral("Server 1"));
        QCOMPARE(m_model.data(1, ServersModel::NameRole).toString(), QStringLiteral("Server 2"));
        QCOMPARE(m_model.data(0, ServersModel::HostNameRole).toString(), QStringLiteral("one.example.com"));
        QCOMPARE(m_model.data(1, ServersModel::ServerIdRole).toString(), QStringLiteral("server-2"));
        QVERIFY(!m_model.data(0, ServersModel::IsDefaultRole).toBool());
        QVERIFY(m_model.data(1, ServersModel::IsDefaultRole).toBool());
    }

    void testServerDescription()
    {
        auto server = createServer("server-1", "Test server", "vpn.example.com");

        m_model.updateModel({server}, QString());

        QCOMPARE(m_model.data(0, ServersModel::ServerDescriptionRole).toString(), QStringLiteral("VPN | vpn.example.com"));

        server.configVersion = 1;
        server.baseDescription = QStringLiteral("API server");

        m_model.updateModel({server}, QString());

        QCOMPARE(m_model.data(0, ServersModel::ServerDescriptionRole).toString(), QStringLiteral("API server"));
    }

    void testInvalidIndexes()
    {
        const auto server = createServer("server-1", "Test server", "vpn.example.com");

        m_model.updateModel({server}, QString());

        QVERIFY(!m_model.data(QModelIndex(), ServersModel::NameRole).isValid());
        QVERIFY(!m_model.data(-1, ServersModel::NameRole).isValid());
        QVERIFY(!m_model.data(1, ServersModel::NameRole).isValid());
        QVERIFY(!m_model.data(0, Qt::DisplayRole).isValid());
    }

    void testSetDefaultServerId()
    {
        const QVector<ServerDescription> servers = {
            createServer("server-1", "Server 1", "one.example.com"),
            createServer("server-2", "Server 2", "two.example.com")
        };

        m_model.updateModel(servers, "server-1");

        QSignalSpy changedSpy(&m_model, &QAbstractItemModel::dataChanged);

        m_model.setDefaultServerId("server-2");

        QVERIFY(!m_model.data(0, ServersModel::IsDefaultRole).toBool());
        QVERIFY(m_model.data(1, ServersModel::IsDefaultRole).toBool());

        QCOMPARE(changedSpy.count(), 2);

        for (const auto &arguments : changedSpy) {
            QCOMPARE(arguments.at(2).value<QVector<int>>(), QVector<int>{ServersModel::IsDefaultRole});
        }
    }

    void testSetSameDefaultServerId()
    {
        const auto server = createServer("server-1", "Server 1", "one.example.com");

        m_model.updateModel({server}, "server-1");

        QSignalSpy changedSpy(&m_model, &QAbstractItemModel::dataChanged);

        m_model.setDefaultServerId("server-1");

        QCOMPARE(changedSpy.count(), 0);
        QVERIFY(m_model.data(0, ServersModel::IsDefaultRole).toBool());
    }

    void testSetUnknownDefaultServerId()
    {
        const auto server = createServer("server-1", "Server 1", "one.example.com");

        m_model.updateModel({server}, "server-1");

        QSignalSpy changedSpy(&m_model, &QAbstractItemModel::dataChanged);

        m_model.setDefaultServerId("unknown-server");

        QVERIFY(!m_model.data(0, ServersModel::IsDefaultRole).toBool());
        QCOMPARE(changedSpy.count(), 1);
    }

    void testRemoveAllServers()
    {
        const auto server = createServer("server-1", "Server 1", "one.example.com");

        m_model.updateModel({server}, "server-1");
        QCOMPARE(m_model.rowCount(), 1);

        m_model.updateModel({}, QString());

        QCOMPARE(m_model.rowCount(), 0);
        QVERIFY(!m_model.data(0, ServersModel::NameRole).isValid());
    }

    void testModelConsistency()
    {
        QAbstractItemModelTester tester(&m_model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        m_model.updateModel({
            createServer("server-1", "Server 1", "one.example.com"),
            createServer("server-2", "Server 2", "two.example.com")
        }, "server-1");

        m_model.setDefaultServerId("server-2");
        m_model.updateModel({}, QString());
    }
};

QTEST_MAIN(TestServersModel)
#include "testServersModel.moc"
