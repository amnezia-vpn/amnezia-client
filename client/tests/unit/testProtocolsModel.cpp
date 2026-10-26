#include <QSignalSpy>
#include <QTest>

#include "core/utils/containerEnum.h"
#include "core/utils/protocolEnum.h"
#include "ui/models/protocolsModel.h"

using namespace amnezia;

class TestProtocolsModel : public QObject
{
    Q_OBJECT

private slots:
    void initialState();
    void rowCountDependsOnContainer();
    void updateModel();
    void dataReturnsInvalidForInvalidIndex();
    void dataReturnsInvalidForUnknownRole();
    void protocolTypeRoles();
    void rawConfigNormalizesLineEndings();
    void roleNames();
};

void TestProtocolsModel::initialState()
{
    ProtocolsModel model;

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.rowCount(QModelIndex()), 0);
    QVERIFY(!model.index(0, 0).isValid());
}

void TestProtocolsModel::rowCountDependsOnContainer()
{
    ProtocolsModel model;
    ContainerConfig config { };

    config.container = DockerContainer::None;
    model.updateModel(config);
    QCOMPARE(model.rowCount(), 0);

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);
    QCOMPARE(model.rowCount(), 1);

    config.container = DockerContainer::Awg;
    model.updateModel(config);
    QCOMPARE(model.rowCount(), 1);

    config.container = DockerContainer::OpenVpn;
    model.updateModel(config);
    QCOMPARE(model.rowCount(), 1);
}

void TestProtocolsModel::updateModel()
{
    ProtocolsModel model;
    ContainerConfig config {};

    config.container = DockerContainer::WireGuard;

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QVERIFY(resetSpy.isValid());

    model.updateModel(config);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(resetSpy.count(), 1);

    config.container = DockerContainer::None;
    model.updateModel(config);

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(resetSpy.count(), 2);
}

void TestProtocolsModel::dataReturnsInvalidForInvalidIndex()
{
    ProtocolsModel model;
    ContainerConfig config {};

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);

    QVERIFY(!model.data(QModelIndex(), ProtocolsModel::ProtocolNameRole).isValid());
    QVERIFY(!model.data(model.index(-1, 0), ProtocolsModel::ProtocolNameRole).isValid());
    QVERIFY(!model.data(model.index(1, 0), ProtocolsModel::ProtocolNameRole).isValid());
}

void TestProtocolsModel::dataReturnsInvalidForUnknownRole()
{
    ProtocolsModel model;
    ContainerConfig config {};

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);

    const QModelIndex index = model.index(0, 0);
    QVERIFY(index.isValid());

    QVERIFY(!model.data(index, Qt::UserRole + 1000).isValid());
}

void TestProtocolsModel::protocolTypeRoles()
{
    ProtocolsModel model;
    ContainerConfig config { };

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);

    const QModelIndex index = model.index(0, 0);

    QCOMPARE(model.data(index, ProtocolsModel::IsWireGuardRole).toBool(), true);
    QCOMPARE(model.data(index, ProtocolsModel::IsAwgRole).toBool(), false);
    QCOMPARE(model.data(index, ProtocolsModel::IsOpenVpnRole).toBool(), false);
    QCOMPARE(model.data(index, ProtocolsModel::IsXrayRole).toBool(), false);
}

void TestProtocolsModel::rawConfigNormalizesLineEndings()
{
    ProtocolsModel model;
    ContainerConfig config {};

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);

    const QModelIndex index = model.index(0, 0);

    // A default configuration has no native client configuration.
    QCOMPARE(model.data(index, ProtocolsModel::RawConfigRole).toString(), QStringLiteral("\n"));
}

void TestProtocolsModel::roleNames()
{
    ProtocolsModel model;
    const auto roles = model.roleNames();

    QCOMPARE(roles.value(ProtocolsModel::ProtocolNameRole), QByteArray("protocolName"));
    QCOMPARE(roles.value(ProtocolsModel::ServerProtocolPageRole), QByteArray("serverProtocolPage"));
    QCOMPARE(roles.value(ProtocolsModel::ClientProtocolPageRole), QByteArray("clientProtocolPage"));
    QCOMPARE(roles.value(ProtocolsModel::ProtocolIndexRole), QByteArray("protocolIndex"));
    QCOMPARE(roles.value(ProtocolsModel::ProtocolStringRole), QByteArray("protocolString"));
    QCOMPARE(roles.value(ProtocolsModel::RawConfigRole), QByteArray("rawConfig"));
    QCOMPARE(roles.value(ProtocolsModel::IsClientProtocolExistsRole), QByteArray("isClientProtocolExists"));
    QCOMPARE(roles.value(ProtocolsModel::IsWireGuardRole), QByteArray("isWireGuard"));
    QCOMPARE(roles.value(ProtocolsModel::IsAwgRole), QByteArray("isAwg"));
    QCOMPARE(roles.value(ProtocolsModel::IsOpenVpnRole), QByteArray("isOpenVpn"));
    QCOMPARE(roles.value(ProtocolsModel::IsXrayRole), QByteArray("isXray"));
}

QTEST_MAIN(TestProtocolsModel)
#include "testProtocolsModel.moc"
