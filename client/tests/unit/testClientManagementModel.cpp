#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

#include "core/utils/constants/configKeys.h"
#include "ui/models/clientManagementModel.h"

using namespace amnezia;

class TestableClientManagementModel : public ClientManagementModel
{
public:
    using ClientManagementModel::ClientManagementModel;
    using ClientManagementModel::roleNames;
};

class TestClientManagementModel : public QObject
{
    Q_OBJECT

private:
    static QJsonObject createClient(const QString &clientId = QStringLiteral("client-1"),
                                    const QString &clientName = QStringLiteral("Test client"))
    {
        QJsonObject userData;
        userData[configKey::clientName] = clientName;
        userData[configKey::creationDate] = QStringLiteral("2026-01-15");
        userData[configKey::latestHandshake] = QStringLiteral("2026-01-16 12:30:00");
        userData[configKey::dataReceived] = QStringLiteral("1024");
        userData[configKey::dataSent] = QStringLiteral("2048");
        userData[configKey::allowedIps] = QStringLiteral("10.0.0.2/32");

        QJsonObject client;
        client[configKey::clientId] = clientId;
        client[configKey::userData] = userData;

        return client;
    }

private slots:
    void initTestCase()
    {
        qRegisterMetaType<QModelIndex>("QModelIndex");
        qRegisterMetaType<QVector<int>>("QVector<int>");
    }

    void emptyModel()
    {
        ClientManagementModel model;

        QCOMPARE(model.rowCount(), 0);
        QVERIFY(!model.index(0, 0).isValid());
    }

    void updateModel()
    {
        ClientManagementModel model;

        QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
        QVERIFY(resetSpy.isValid());

        const QJsonArray clients { createClient(),
                                   createClient(QStringLiteral("client-2"), QStringLiteral("Second client")) };

        model.updateModel(clients);

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(resetSpy.count(), 1);
    }

    void updateModelWithEmptyArray()
    {
        ClientManagementModel model;
        model.updateModel(QJsonArray { createClient() });

        QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
        QVERIFY(resetSpy.isValid());

        model.updateModel(QJsonArray());

        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(resetSpy.count(), 1);
    }

    void dataReturnsAllRoles()
    {
        ClientManagementModel model;
        model.updateModel(QJsonArray { createClient() });

        const QModelIndex index = model.index(0, 0);
        QVERIFY(index.isValid());

        QCOMPARE(model.data(index, ClientManagementModel::ClientIdRole).toString(), QStringLiteral("client-1"));
        QCOMPARE(model.data(index, ClientManagementModel::ClientNameRole).toString(), QStringLiteral("Test client"));
        QCOMPARE(model.data(index, ClientManagementModel::CreationDateRole).toString(), QStringLiteral("2026-01-15"));
        QCOMPARE(model.data(index, ClientManagementModel::LatestHandshakeRole).toString(), QStringLiteral("2026-01-16 12:30:00"));
        QCOMPARE(model.data(index, ClientManagementModel::DataReceivedRole).toString(), QStringLiteral("1024"));
        QCOMPARE(model.data(index, ClientManagementModel::DataSentRole).toString(), QStringLiteral("2048"));
        QCOMPARE(model.data(index, ClientManagementModel::AllowedIpsRole).toString(), QStringLiteral("10.0.0.2/32"));
    }

    void dataReturnsInvalidVariantForInvalidIndexes()
    {
        ClientManagementModel model;
        model.updateModel(QJsonArray { createClient() });

        QVERIFY(!model.data(QModelIndex(), ClientManagementModel::ClientIdRole).isValid());
        QVERIFY(!model.data(model.index(-1, 0), ClientManagementModel::ClientIdRole).isValid());
        QVERIFY(!model.data(model.index(1, 0), ClientManagementModel::ClientIdRole).isValid());
    }

    void dataReturnsInvalidVariantForUnknownRole()
    {
        ClientManagementModel model;
        model.updateModel(QJsonArray { createClient() });

        const QModelIndex index = model.index(0, 0);
        const int unknownRole = ClientManagementModel::AllowedIpsRole + 1;

        QVERIFY(!model.data(index, unknownRole).isValid());
    }

    void updateClientName()
    {
        ClientManagementModel model;
        model.updateModel(QJsonArray { createClient() });

        QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);
        QVERIFY(dataChangedSpy.isValid());

        model.updateClientName(0, QStringLiteral("Renamed client"));

        QCOMPARE(model.data(model.index(0, 0), ClientManagementModel::ClientNameRole).toString(), QStringLiteral("Renamed client"));
        QCOMPARE(model.data(model.index(0, 0), ClientManagementModel::ClientIdRole).toString(), QStringLiteral("client-1"));
        QCOMPARE(dataChangedSpy.count(), 1);

        const QList<QVariant> arguments = dataChangedSpy.takeFirst();

        QCOMPARE(qvariant_cast<QModelIndex>(arguments.at(0)), model.index(0, 0));
        QCOMPARE(qvariant_cast<QModelIndex>(arguments.at(1)), model.index(0, 0));

        const QVector<int> roles = qvariant_cast<QVector<int>>(arguments.at(2));

        QCOMPARE(roles, QVector<int> { ClientManagementModel::ClientNameRole });
    }

    void updateClientNameWithEmptyString()
    {
        ClientManagementModel model;
        model.updateModel(QJsonArray { createClient() });

        model.updateClientName(0, QString());

        QCOMPARE(model.data(model.index(0, 0), ClientManagementModel::ClientNameRole).toString(), QString());
        QCOMPARE(model.rowCount(), 1);
    }

    void updateClientNameWithInvalidRow()
    {
        ClientManagementModel model;
        model.updateModel(QJsonArray { createClient() });

        QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);
        QVERIFY(dataChangedSpy.isValid());

        model.updateClientName(-1, QStringLiteral("Invalid"));
        model.updateClientName(1, QStringLiteral("Invalid"));

        QCOMPARE(dataChangedSpy.count(), 0);
        QCOMPARE(model.data(model.index(0, 0), ClientManagementModel::ClientNameRole).toString(),
                 QStringLiteral("Test client"));
    }

    void dataReturnsEmptyStringsForMissingFields()
    {
        ClientManagementModel model;

        QJsonObject client;
        client[configKey::clientId] = QStringLiteral("client-1");

        model.updateModel(QJsonArray { client });

        const QModelIndex index = model.index(0, 0);

        QCOMPARE(model.data(index, ClientManagementModel::ClientIdRole).toString(), QStringLiteral("client-1"));
        QCOMPARE(model.data(index, ClientManagementModel::ClientNameRole).toString(), QString());
        QCOMPARE(model.data(index, ClientManagementModel::CreationDateRole).toString(), QString());
        QCOMPARE(model.data(index, ClientManagementModel::LatestHandshakeRole).toString(), QString());
        QCOMPARE(model.data(index, ClientManagementModel::DataReceivedRole).toString(), QString());
        QCOMPARE(model.data(index, ClientManagementModel::DataSentRole).toString(), QString());
        QCOMPARE(model.data(index, ClientManagementModel::AllowedIpsRole).toString(), QString());
    }

    void roleNamesAreCorrect()
    {
        TestableClientManagementModel model;
        const QHash<int, QByteArray> roles = model.roleNames();

        QCOMPARE(roles.value(ClientManagementModel::ClientIdRole), QByteArray("clientId"));
        QCOMPARE(roles.value(ClientManagementModel::ClientNameRole), QByteArray("clientName"));
        QCOMPARE(roles.value(ClientManagementModel::CreationDateRole), QByteArray("creationDate"));
        QCOMPARE(roles.value(ClientManagementModel::LatestHandshakeRole), QByteArray("latestHandshake"));
        QCOMPARE(roles.value(ClientManagementModel::DataReceivedRole), QByteArray("dataReceived"));
        QCOMPARE(roles.value(ClientManagementModel::DataSentRole), QByteArray("dataSent"));
        QCOMPARE(roles.value(ClientManagementModel::AllowedIpsRole), QByteArray("allowedIps"));
    }
};

QTEST_MAIN(TestClientManagementModel)
#include "testClientManagementModel.moc"
