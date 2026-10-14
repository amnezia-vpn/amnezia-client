#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

#include "ui/models/api/apiBenefitsModel.h"

class TestApiBenefitsModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void initialState();
    void updateModel();
    void updateModelWithEmptyArray();
    void updateModelSkipsInvalidItems();
    void updateModelSkipsEmptyItems();
    void updateModelTrimsLinkedBody();
    void updateModelPreservesQrcIcon();
    void updateModelUsesFallbackIcon();
    void dataReturnsInvalidForInvalidIndex();
    void dataReturnsInvalidForUnknownRole();
    void roleNames();
    void clear();
};

void TestApiBenefitsModel::initTestCase()
{
    qRegisterMetaType<QModelIndex>();
}

void TestApiBenefitsModel::initialState()
{
    ApiBenefitsModel model;

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.rowCount(QModelIndex()), 0);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);
}

void TestApiBenefitsModel::updateModel()
{
    ApiBenefitsModel model;

    const QJsonArray benefits = { QJsonObject { { "icon", "globe-2" },
                                                { "title", "Global access" },
                                                { "body", "Access servers worldwide" },
                                                { "link", false } },
                                  QJsonObject { { "icon", "smartphone" },
                                                { "title", "Multiple devices" },
                                                { "body", "Connect your devices" },
                                                { "link", true } } };

    QSignalSpy aboutToResetSpy(&model, &QAbstractItemModel::modelAboutToBeReset);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.updateModel(benefits);

    QCOMPARE(aboutToResetSpy.count(), 1);
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 2);

    const QModelIndex firstIndex = model.index(0, 0);
    QVERIFY(firstIndex.isValid());

    QCOMPARE(model.data(firstIndex, ApiBenefitsModel::IconRole).toString(), QStringLiteral("qrc:/images/controls/globe-2.svg"));
    QCOMPARE(model.data(firstIndex, ApiBenefitsModel::TitleRole).toString(), QStringLiteral("Global access"));
    QCOMPARE(model.data(firstIndex, ApiBenefitsModel::BodyRole).toString(), QStringLiteral("Access servers worldwide"));
    QCOMPARE(model.data(firstIndex, ApiBenefitsModel::LinkRole).toBool(), false);

    const QModelIndex secondIndex = model.index(1, 0);
    QVERIFY(secondIndex.isValid());

    QCOMPARE(model.data(secondIndex, ApiBenefitsModel::IconRole).toString(), QStringLiteral("qrc:/images/controls/smartphone.svg"));
    QCOMPARE(model.data(secondIndex, ApiBenefitsModel::TitleRole).toString(), QStringLiteral("Multiple devices"));
    QCOMPARE(model.data(secondIndex, ApiBenefitsModel::BodyRole).toString(), QStringLiteral("Connect your devices"));
    QCOMPARE(model.data(secondIndex, ApiBenefitsModel::LinkRole).toBool(), true);
}

void TestApiBenefitsModel::updateModelWithEmptyArray()
{
    ApiBenefitsModel model;

    model.updateModel({ QJsonObject { { "title", "Benefit" }, { "body", "Description" } } });

    QCOMPARE(model.rowCount(), 1);

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.updateModel(QJsonArray { });

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 0);
}

void TestApiBenefitsModel::updateModelSkipsInvalidItems()
{
    ApiBenefitsModel model;

    const QJsonArray benefits = { QJsonObject { { "title", "Valid benefit" }, { "body", "Valid description" } },
                                  QJsonValue(QStringLiteral("invalid item")), QJsonValue(42), QJsonValue(false),
                                  QJsonValue(QJsonArray { }) };

    model.updateModel(benefits);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), ApiBenefitsModel::TitleRole).toString(), QStringLiteral("Valid benefit"));
}

void TestApiBenefitsModel::updateModelSkipsEmptyItems()
{
    ApiBenefitsModel model;

    const QJsonArray benefits = { QJsonObject { }, QJsonObject { { "title", "" }, { "body", "" } },
                                  QJsonObject { { "title", "Title only" } }, QJsonObject { { "body", "Body only" } } };

    model.updateModel(benefits);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 0), ApiBenefitsModel::TitleRole).toString(), QStringLiteral("Title only"));
    QCOMPARE(model.data(model.index(1, 0), ApiBenefitsModel::BodyRole).toString(), QStringLiteral("Body only"));
}

void TestApiBenefitsModel::updateModelTrimsLinkedBody()
{
    ApiBenefitsModel model;

    model.updateModel({ QJsonObject { { "title", "Link" }, { "body", "  https://example.com  \n" }, { "link", true } },
                        QJsonObject { { "title", "Text" }, { "body", "  Description  " }, { "link", false } } });

    QCOMPARE(model.data(model.index(0, 0), ApiBenefitsModel::BodyRole).toString(), QStringLiteral("https://example.com"));
    QCOMPARE(model.data(model.index(1, 0), ApiBenefitsModel::BodyRole).toString(), QStringLiteral("  Description  "));
}

void TestApiBenefitsModel::updateModelPreservesQrcIcon()
{
    ApiBenefitsModel model;

    model.updateModel({ QJsonObject { { "title", "Benefit" }, { "body", "Description" }, { "icon", "qrc:/custom/icon.svg" } } });

    QCOMPARE(model.data(model.index(0, 0), ApiBenefitsModel::IconRole).toString(), QStringLiteral("qrc:/custom/icon.svg"));
}

void TestApiBenefitsModel::updateModelUsesFallbackIcon()
{
    ApiBenefitsModel model;

    model.updateModel({ QJsonObject { { "title", "Benefit" }, { "body", "Description" }, { "icon", "unknown-icon" } } });

    QCOMPARE(model.data(model.index(0, 0), ApiBenefitsModel::IconRole).toString(), QStringLiteral("qrc:/images/controls/info.svg"));
}

void TestApiBenefitsModel::dataReturnsInvalidForInvalidIndex()
{
    ApiBenefitsModel model;

    model.updateModel({ QJsonObject { { "title", "Benefit" }, { "body", "Description" } } });

    QVERIFY(!model.data(QModelIndex(), ApiBenefitsModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(-1, 0), ApiBenefitsModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(1, 0), ApiBenefitsModel::TitleRole).isValid());
    QVERIFY(!model.data(model.index(0, 1), ApiBenefitsModel::TitleRole).isValid());
}

void TestApiBenefitsModel::dataReturnsInvalidForUnknownRole()
{
    ApiBenefitsModel model;

    model.updateModel({ QJsonObject { { "title", "Benefit" }, { "body", "Description" } } });

    const QModelIndex index = model.index(0, 0);

    QVERIFY(!model.data(index, Qt::DisplayRole).isValid());
    QVERIFY(!model.data(index, ApiBenefitsModel::LinkRole + 1).isValid());
}

void TestApiBenefitsModel::roleNames()
{
    ApiBenefitsModel model;

    const QHash<int, QByteArray> roles = model.roleNames();

    QCOMPARE(roles.value(ApiBenefitsModel::IconRole), QByteArray("icon"));
    QCOMPARE(roles.value(ApiBenefitsModel::TitleRole), QByteArray("title"));
    QCOMPARE(roles.value(ApiBenefitsModel::BodyRole), QByteArray("body"));
    QCOMPARE(roles.value(ApiBenefitsModel::LinkRole), QByteArray("link"));
}

void TestApiBenefitsModel::clear()
{
    ApiBenefitsModel model;

    model.updateModel({ QJsonObject { { "title", "Benefit 1" }, { "body", "Description 1" } },
                        QJsonObject { { "title", "Benefit 2" }, { "body", "Description 2" } } });

    QSignalSpy aboutToResetSpy(&model, &QAbstractItemModel::modelAboutToBeReset);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.clear();

    QCOMPARE(aboutToResetSpy.count(), 1);
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 0);

    model.clear();

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(resetSpy.count(), 2);
}

QTEST_MAIN(TestApiBenefitsModel)
#include "testApiBenefitsModel.moc"
