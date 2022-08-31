import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import QtQml 2.2

Item {
    ListModel {
        id: listModel
        ListElement { name: "1"; age: 18 }
        ListElement { name: "2"; age: 22 }
        ListElement { name: "3"; age: 45 }
        ListElement { name: "4"; age: 10 }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: listModel

        proxyRoles: FilterRole {
            name: "isOldEnough"
            RangeFilter {
                id: ageFilter
                roleName: "age"
                minimumInclusive: true
                minimumValue: 18
            }
        }
    }
    TestCase {
        name: "FilterRole"

        function test_filterRole() {
            compare(testModel.get(0, "isOldEnough"), true);
            compare(testModel.get(1, "isOldEnough"), true);
            compare(testModel.get(2, "isOldEnough"), true);
            compare(testModel.get(3, "isOldEnough"), false);

            ageFilter.minimumValue = 21;

            compare(testModel.get(0, "isOldEnough"), false);
            compare(testModel.get(1, "isOldEnough"), true);
            compare(testModel.get(2, "isOldEnough"), true);
            compare(testModel.get(3, "isOldEnough"), false);
        }
    }
}
