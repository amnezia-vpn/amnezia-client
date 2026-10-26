#include <QSignalSpy>
#include <QTest>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "ui/models/containersModel.h"

using namespace amnezia;

class TestContainersModel : public QObject
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testRowCount();
    void testInvalidIndexes();
    void testRoleNames();
    void testContainerData();
    void testProcessedContainer();
    void testUpdateModel();
    void testInstalledContainers();
};

void TestContainersModel::testInitialState()
{
    ContainersModel model;

    QCOMPARE(model.rowCount(), ContainerUtils::allContainers().size());
    QVERIFY(!model.hasInstalledServices());
    QVERIFY(!model.hasInstalledProtocols());
    QCOMPARE(model.getProcessedContainerName(), QString());
}

void TestContainersModel::testRowCount()
{
    ContainersModel model;

    QCOMPARE(model.rowCount(QModelIndex()), ContainerUtils::allContainers().size());

    const QModelIndex parent = model.index(0, 0);
    QCOMPARE(model.rowCount(parent), 0);
}

void TestContainersModel::testInvalidIndexes()
{
    ContainersModel model;

    QVERIFY(!model.data(QModelIndex(), ContainersModel::NameRole).isValid());
    QVERIFY(!model.data(model.index(-1, 0), ContainersModel::NameRole).isValid());
    QVERIFY(!model.data(model.index(model.rowCount(), 0), ContainersModel::NameRole).isValid());
    QVERIFY(!model.data(-1, ContainersModel::NameRole).isValid());
    QVERIFY(!model.data(model.rowCount(), ContainersModel::NameRole).isValid());
}

void TestContainersModel::testRoleNames()
{
    ContainersModel model;
    const auto roles = model.roleNames();

    QCOMPARE(roles.value(ContainersModel::NameRole), QByteArray("name"));
    QCOMPARE(roles.value(ContainersModel::DescriptionRole), QByteArray("description"));
    QCOMPARE(roles.value(ContainersModel::DetailedDescriptionRole), QByteArray("detailedDescription"));
    QCOMPARE(roles.value(ContainersModel::ConfigRole), QByteArray("config"));
    QCOMPARE(roles.value(ContainersModel::IsInstalledRole), QByteArray("isInstalled"));
    QCOMPARE(roles.value(ContainersModel::IsCurrentlyProcessedRole), QByteArray("isCurrentlyProcessed"));
    QCOMPARE(roles.value(ContainersModel::IsSupportedRole), QByteArray("isSupported"));
    QCOMPARE(roles.value(ContainersModel::IsVpnContainerRole), QByteArray("isVpnContainer"));
    QCOMPARE(roles.value(ContainersModel::IsServiceContainerRole), QByteArray("isServiceContainer"));
    QCOMPARE(roles.value(ContainersModel::ContainerStringRole), QByteArray("containerString"));
}

void TestContainersModel::testContainerData()
{
    ContainersModel model;
    const auto containers = ContainerUtils::allContainers();

    QVERIFY(!containers.isEmpty());

    for (int row = 0; row < containers.size(); ++row) {
        const QModelIndex index = model.index(row, 0);
        QVERIFY(index.isValid());

        const DockerContainer container = containers.at(row);

        QCOMPARE(model.data(index, ContainersModel::DockerContainerRole).toInt(), static_cast<int>(container));
        QCOMPARE(model.data(row, ContainersModel::ContainerStringRole).toString(), ContainerUtils::containerToString(container));
        QCOMPARE(model.data(index, ContainersModel::ServiceTypeRole).toInt(), static_cast<int>(ContainerUtils::containerService(container)));
        QCOMPARE(model.data(index, ContainersModel::IsVpnContainerRole).toBool(), ContainerUtils::containerService(container) == ProtocolEnumNS::ServiceType::Vpn);
        QCOMPARE(model.data(index, ContainersModel::IsServiceContainerRole).toBool(), ContainerUtils::containerService(container) == ProtocolEnumNS::ServiceType::Other);
        QCOMPARE(model.data(index, ContainersModel::IsInstalledRole).toBool(), false);
        QCOMPARE(model.data(index, ContainersModel::IsThirdPartyConfigRole).toBool(), false);
    }
}

void TestContainersModel::testProcessedContainer()
{
    ContainersModel model;
    const auto containers = ContainerUtils::allContainers();

    for (int row = 0; row < containers.size(); ++row) {
        QVERIFY(!model.data(model.index(row, 0), ContainersModel::IsCurrentlyProcessedRole).toBool());
    }

    for (const DockerContainer container : containers) {
        model.setProcessedContainerIndex(static_cast<int>(container));

        QCOMPARE(model.getProcessedContainerName(), ContainerUtils::containerHumanNames().value(container));

        for (int row = 0; row < containers.size(); ++row) {
            const bool expected = containers.at(row) == container;

            QCOMPARE(model.data(model.index(row, 0), ContainersModel::IsCurrentlyProcessedRole).toBool(), expected);
        }
    }
}

void TestContainersModel::testUpdateModel()
{
    ContainersModel model;

    QSignalSpy aboutToResetSpy(&model, &QAbstractItemModel::modelAboutToBeReset);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    QVERIFY(aboutToResetSpy.isValid());
    QVERIFY(resetSpy.isValid());

    model.updateModel({});

    QCOMPARE(aboutToResetSpy.count(), 1);
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), ContainerUtils::allContainers().size());

    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, 0);

        QVERIFY(!model.data(index, ContainersModel::IsInstalledRole).toBool());
        QVERIFY(model.data(index, ContainersModel::ConfigRole).toJsonObject().isEmpty());
    }
}

void TestContainersModel::testInstalledContainers()
{
    ContainersModel model;

    model.updateModel({});

    QVERIFY(!model.hasInstalledServices());
    QVERIFY(!model.hasInstalledProtocols());

    for (int row = 0; row < model.rowCount(); ++row) {
        QVERIFY(model.getContainerConfig(row).isEmpty());
        QVERIFY(!model.isServiceContainer(row));
    }
}

QTEST_GUILESS_MAIN(TestContainersModel)
#include "testContainersModel.moc"
