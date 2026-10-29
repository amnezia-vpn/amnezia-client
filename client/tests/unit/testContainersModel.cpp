#include <QSignalSpy>
#include <QTest>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "ui/models/containersModel.h"

using namespace amnezia;

namespace
{
    class TestableContainersModel : public ContainersModel
    {
    public:
        using ContainersModel::ContainersModel;
        using ContainersModel::roleNames;
    };
} // namespace

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
    TestableContainersModel model;

    QCOMPARE(model.rowCount(), ContainerUtils::allContainers().size());
    QVERIFY(!model.hasInstalledServices());
    QVERIFY(!model.hasInstalledProtocols());
    QCOMPARE(model.getProcessedContainerName(), QString());
}

void TestContainersModel::testRowCount()
{
    TestableContainersModel model;

    const int expectedCount = ContainerUtils::allContainers().size();

    QCOMPARE(model.rowCount(), expectedCount);
    QCOMPARE(model.rowCount(QModelIndex()), expectedCount);

    const QModelIndex parent = model.index(0, 0);
    QVERIFY(parent.isValid());
    QCOMPARE(model.rowCount(parent), expectedCount);
}

void TestContainersModel::testInvalidIndexes()
{
    TestableContainersModel model;

    QVERIFY(!model.data(QModelIndex(), TestableContainersModel::NameRole).isValid());
    QVERIFY(!model.data(model.index(-1, 0), TestableContainersModel::NameRole).isValid());
    QVERIFY(!model.data(model.index(model.rowCount(), 0), TestableContainersModel::NameRole).isValid());
    QVERIFY(!model.data(-1, TestableContainersModel::NameRole).isValid());
    QVERIFY(!model.data(model.rowCount(), TestableContainersModel::NameRole).isValid());
}

void TestContainersModel::testRoleNames()
{
    TestableContainersModel model;
    const auto roles = model.roleNames();

    QCOMPARE(roles.value(TestableContainersModel::NameRole), QByteArray("name"));
    QCOMPARE(roles.value(TestableContainersModel::DescriptionRole), QByteArray("description"));
    QCOMPARE(roles.value(TestableContainersModel::DetailedDescriptionRole), QByteArray("detailedDescription"));
    QCOMPARE(roles.value(TestableContainersModel::ConfigRole), QByteArray("config"));
    QCOMPARE(roles.value(TestableContainersModel::IsInstalledRole), QByteArray("isInstalled"));
    QCOMPARE(roles.value(TestableContainersModel::IsCurrentlyProcessedRole), QByteArray("isCurrentlyProcessed"));
    QCOMPARE(roles.value(TestableContainersModel::IsSupportedRole), QByteArray("isSupported"));
    QCOMPARE(roles.value(TestableContainersModel::IsVpnContainerRole), QByteArray("isVpnContainer"));
    QCOMPARE(roles.value(TestableContainersModel::IsServiceContainerRole), QByteArray("isServiceContainer"));
    QCOMPARE(roles.value(TestableContainersModel::ContainerStringRole), QByteArray("containerString"));
}

void TestContainersModel::testContainerData()
{
    TestableContainersModel model;
    const auto containers = ContainerUtils::allContainers();

    QVERIFY(!containers.isEmpty());

    for (int row = 0; row < containers.size(); ++row) {
        const QModelIndex index = model.index(row, 0);
        QVERIFY(index.isValid());

        const DockerContainer container = containers.at(row);

        QCOMPARE(model.data(index, TestableContainersModel::DockerContainerRole).toInt(), static_cast<int>(container));
        QCOMPARE(model.data(row, TestableContainersModel::ContainerStringRole).toString(), ContainerUtils::containerToString(container));
        QCOMPARE(model.data(index, TestableContainersModel::ServiceTypeRole).toInt(), static_cast<int>(ContainerUtils::containerService(container)));
        QCOMPARE(model.data(index, TestableContainersModel::IsVpnContainerRole).toBool(), ContainerUtils::containerService(container) == ProtocolEnumNS::ServiceType::Vpn);
        QCOMPARE(model.data(index, TestableContainersModel::IsServiceContainerRole).toBool(), ContainerUtils::containerService(container) == ProtocolEnumNS::ServiceType::Other);
        QCOMPARE(model.data(index, TestableContainersModel::IsInstalledRole).toBool(), false);
        QCOMPARE(model.data(index, TestableContainersModel::IsThirdPartyConfigRole).toBool(), false);
    }
}

void TestContainersModel::testProcessedContainer()
{
    TestableContainersModel model;
    const auto containers = ContainerUtils::allContainers();

    for (int row = 0; row < containers.size(); ++row) {
        QVERIFY(!model.data(model.index(row, 0), TestableContainersModel::IsCurrentlyProcessedRole).toBool());
    }

    for (const DockerContainer container : containers) {
        model.setProcessedContainerIndex(static_cast<int>(container));

        QCOMPARE(model.getProcessedContainerName(), ContainerUtils::containerHumanNames().value(container));

        for (int row = 0; row < containers.size(); ++row) {
            const bool expected = containers.at(row) == container;

            QCOMPARE(model.data(model.index(row, 0), TestableContainersModel::IsCurrentlyProcessedRole).toBool(), expected);
        }
    }
}

void TestContainersModel::testUpdateModel()
{
    TestableContainersModel model;

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

        QVERIFY(!model.data(index, TestableContainersModel::IsInstalledRole).toBool());
        QVERIFY(model.data(index, TestableContainersModel::ConfigRole).toJsonObject().isEmpty());
    }
}

void TestContainersModel::testInstalledContainers()
{
    TestableContainersModel model;

    model.updateModel({});

    QVERIFY(!model.hasInstalledServices());
    QVERIFY(!model.hasInstalledProtocols());

    const auto containers = ContainerUtils::allContainers();

    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, 0);
        QVERIFY(index.isValid());
        QVERIFY(model.getContainerConfig(row).isEmpty());

        QCOMPARE(model.data(index, TestableContainersModel::IsInstalledRole).toBool(), false);

        const auto container = containers.at(row);
        const bool expectedService = ContainerUtils::containerService(container) == ProtocolEnumNS::ServiceType::Other;

        QCOMPARE(model.isServiceContainer(row), expectedService);
    }
}


QTEST_GUILESS_MAIN(TestContainersModel)
#include "testContainersModel.moc"
