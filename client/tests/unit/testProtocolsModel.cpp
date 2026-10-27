#include <QSignalSpy>
#include <QTest>

#include "core/utils/containerEnum.h"
#include "core/utils/protocolEnum.h"
#include "ui/models/protocolsModel.h"

using namespace amnezia;

namespace
{
    class TestableProtocolsModel : public ProtocolsModel
    {
    public:
        using ProtocolsModel::ProtocolsModel;
        using ProtocolsModel::roleNames;
    };
} // namespace

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
    TestableProtocolsModel model;

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.rowCount(QModelIndex()), 0);
    QVERIFY(!model.index(0, 0).isValid());
}

void TestProtocolsModel::rowCountDependsOnContainer()
{
    TestableProtocolsModel model;
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
    TestableProtocolsModel model;
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
    TestableProtocolsModel model;
    ContainerConfig config {};

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);

    QVERIFY(!model.data(QModelIndex(), TestableProtocolsModel::ProtocolNameRole).isValid());
    QVERIFY(!model.data(model.index(-1, 0), TestableProtocolsModel::ProtocolNameRole).isValid());
    QVERIFY(!model.data(model.index(1, 0), TestableProtocolsModel::ProtocolNameRole).isValid());
}

void TestProtocolsModel::dataReturnsInvalidForUnknownRole()
{
    TestableProtocolsModel model;
    ContainerConfig config {};

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);

    const QModelIndex index = model.index(0, 0);
    QVERIFY(index.isValid());

    QVERIFY(!model.data(index, Qt::UserRole + 1000).isValid());
}

void TestProtocolsModel::protocolTypeRoles()
{
    TestableProtocolsModel model;
    ContainerConfig config {};

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);

    const QModelIndex index = model.index(0, 0);

    QCOMPARE(model.data(index, TestableProtocolsModel::IsWireGuardRole).toBool(), true);
    QCOMPARE(model.data(index, TestableProtocolsModel::IsAwgRole).toBool(), false);
    QCOMPARE(model.data(index, TestableProtocolsModel::IsOpenVpnRole).toBool(), false);
    QCOMPARE(model.data(index, TestableProtocolsModel::IsXrayRole).toBool(), false);
}

void TestProtocolsModel::rawConfigNormalizesLineEndings()
{
    TestableProtocolsModel model;
    ContainerConfig config {};

    config.container = DockerContainer::WireGuard;
    model.updateModel(config);

    const QModelIndex index = model.index(0, 0);

    // A default configuration has no native client configuration.
    QCOMPARE(model.data(index, TestableProtocolsModel::RawConfigRole).toString(), QStringLiteral("\n"));
}

void TestProtocolsModel::roleNames()
{
    TestableProtocolsModel model;
    const auto roles = model.roleNames();

    QCOMPARE(roles.value(TestableProtocolsModel::ProtocolNameRole), QByteArray("protocolName"));
    QCOMPARE(roles.value(TestableProtocolsModel::ServerProtocolPageRole), QByteArray("serverProtocolPage"));
    QCOMPARE(roles.value(TestableProtocolsModel::ClientProtocolPageRole), QByteArray("clientProtocolPage"));
    QCOMPARE(roles.value(TestableProtocolsModel::ProtocolIndexRole), QByteArray("protocolIndex"));
    QCOMPARE(roles.value(TestableProtocolsModel::ProtocolStringRole), QByteArray("protocolString"));
    QCOMPARE(roles.value(TestableProtocolsModel::RawConfigRole), QByteArray("rawConfig"));
    QCOMPARE(roles.value(TestableProtocolsModel::IsClientProtocolExistsRole), QByteArray("isClientProtocolExists"));
    QCOMPARE(roles.value(TestableProtocolsModel::IsWireGuardRole), QByteArray("isWireGuard"));
    QCOMPARE(roles.value(TestableProtocolsModel::IsAwgRole), QByteArray("isAwg"));
    QCOMPARE(roles.value(TestableProtocolsModel::IsOpenVpnRole), QByteArray("isOpenVpn"));
    QCOMPARE(roles.value(TestableProtocolsModel::IsXrayRole), QByteArray("isXray"));
}

QTEST_MAIN(TestProtocolsModel)
#include "testProtocolsModel.moc"
