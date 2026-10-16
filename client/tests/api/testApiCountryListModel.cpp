#include <QSignalSpy>
#include <QTest>

#include "ui/models/api/apiCountryListModel.h"

class TestApiCountryListModel : public QObject
{
    Q_OBJECT

private slots:
    void initialState();
    void dataReturnsInvalidForInvalidIndex();
    void searchTextChanges();
    void searchTextChangedSignal();
    void clearSearch();
    void sortModeChanges();
    void invalidSortModeFallsBackToByRegion();
    void favoritesAreSortedAndDeduplicated();
    void collapsedSectionsAreStored();
    void sanitizeSearchFieldText();
};

void TestApiCountryListModel::initialState()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.rowCount(QModelIndex()), 0);
    QVERIFY(!model.hasResults());
    QVERIFY(!model.isSearchActive());
    QVERIFY(model.searchText().isEmpty());
    QCOMPARE(model.sortMode(), ApiCountryListModel::ByRegion);
    QVERIFY(model.isGrouped());
    QCOMPARE(model.listId(), QStringLiteral("test"));
    QCOMPARE(model.activeUseCaseId(), QStringLiteral("all"));
    QCOMPARE(model.favoritesLimit(), 15);
}

void TestApiCountryListModel::dataReturnsInvalidForInvalidIndex()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(model.index(-1, 0), ApiCountryListModel::CountryNameRole).isValid());
    QVERIFY(!model.data(model.index(0, 0), ApiCountryListModel::CountryNameRole).isValid());
}

void TestApiCountryListModel::searchTextChanges()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    model.setSearchText(QStringLiteral("Estonia"));

    QCOMPARE(model.searchText(), QStringLiteral("Estonia"));
    QVERIFY(model.isSearchActive());
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(!model.hasResults());

    model.setSearchText(QStringLiteral("   "));

    QVERIFY(!model.isSearchActive());
    QCOMPARE(model.searchText(), QStringLiteral("   "));
}

void TestApiCountryListModel::searchTextChangedSignal()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));
    QSignalSpy spy(&model, &ApiCountryListModel::searchTextChanged);

    QVERIFY(spy.isValid());

    model.setSearchText(QStringLiteral("Estonia"));
    QCOMPARE(spy.count(), 1);

    model.setSearchText(QStringLiteral("Estonia"));
    QCOMPARE(spy.count(), 1);

    model.setSearchText(QStringLiteral("Latvia"));
    QCOMPARE(spy.count(), 2);
}

void TestApiCountryListModel::clearSearch()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    model.setSearchText(QStringLiteral("Estonia"));
    QVERIFY(model.isSearchActive());

    QSignalSpy spy(&model, &ApiCountryListModel::searchTextChanged);
    QVERIFY(spy.isValid());

    model.clearSearch();

    QVERIFY(model.searchText().isEmpty());
    QVERIFY(!model.isSearchActive());
    QCOMPARE(spy.count(), 1);

    model.clearSearch();
    QCOMPARE(spy.count(), 1);
}

void TestApiCountryListModel::sortModeChanges()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    QSignalSpy sortSpy(&model, &ApiCountryListModel::sortModeChanged);
    QSignalSpy groupingSpy(&model, &ApiCountryListModel::groupingChanged);

    QVERIFY(sortSpy.isValid());
    QVERIFY(groupingSpy.isValid());

    model.setSortMode(ApiCountryListModel::Alphabetical);

    QCOMPARE(model.sortMode(), ApiCountryListModel::Alphabetical);
    QVERIFY(!model.isGrouped());
    QCOMPARE(sortSpy.count(), 1);
    QCOMPARE(groupingSpy.count(), 1);

    model.setSortMode(ApiCountryListModel::Alphabetical);

    QCOMPARE(sortSpy.count(), 1);
    QCOMPARE(groupingSpy.count(), 1);
}

void TestApiCountryListModel::invalidSortModeFallsBackToByRegion()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    model.setSortMode(ApiCountryListModel::Alphabetical);
    model.setSortMode(999);

    QCOMPARE(model.sortMode(), ApiCountryListModel::ByRegion);
    QVERIFY(model.isGrouped());
}

void TestApiCountryListModel::favoritesAreSortedAndDeduplicated()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    model.setFavorites({ QStringLiteral("US"), QStringLiteral("DE"), QStringLiteral("US"), QString() });

    QCOMPARE(model.favorites(), QStringList({ QStringLiteral("DE"), QStringLiteral("US") }));
}

void TestApiCountryListModel::collapsedSectionsAreStored()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    model.setCollapsedSections({ QStringLiteral("europe"), QStringLiteral("asia"), QStringLiteral("europe"), QString() });

    QCOMPARE(model.collapsedSections(), QStringList({ QStringLiteral("asia"), QStringLiteral("europe") }));
}

void TestApiCountryListModel::sanitizeSearchFieldText()
{
    ApiCountryListModel model(nullptr, QStringLiteral("test"));

    QCOMPARE(model.sanitizeSearchFieldText(QStringLiteral("Estonia")), QStringLiteral("Estonia"));
    QCOMPARE(model.sanitizeSearchFieldText(QStringLiteral("Hello, World! 123")), QStringLiteral("Hello World "));
    QCOMPARE(model.sanitizeSearchFieldText(QStringLiteral("")), QString());
    QCOMPARE(model.sanitizeSearchFieldText(QStringLiteral("123")), QString());
}

QTEST_MAIN(TestApiCountryListModel)
#include "testApiCountryListModel.moc"