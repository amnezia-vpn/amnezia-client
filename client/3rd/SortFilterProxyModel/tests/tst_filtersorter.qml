import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import QtQml 2.2

Item {
    ListModel {
        id: listModel
        ListElement { name: "1"; favorite: true }
        ListElement { name: "2"; favorite: false }
        ListElement { name: "3"; favorite: false }
        ListElement { name: "4"; favorite: true }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: listModel

        sorters: FilterSorter {
            ValueFilter {
                id: favoriteFilter
                roleName: "favorite"
                value: true
            }
        }
    }
    TestCase {
        name: "FilterSorter"

        function test_filterSorter() {
            compare(testModel.get(0, "name"), "1");
            compare(testModel.get(1, "name"), "4");
            compare(testModel.get(2, "name"), "2");
            compare(testModel.get(3, "name"), "3");

            favoriteFilter.value = false;

            compare(testModel.get(0, "name"), "2");
            compare(testModel.get(1, "name"), "3");
            compare(testModel.get(2, "name"), "1");
            compare(testModel.get(3, "name"), "4");
        }
    }
}
