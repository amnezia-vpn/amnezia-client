import QtQuick 2.0
import SortFilterProxyModel 0.2
import QtQml.Models 2.2
import QtTest 1.1

Item {
    property list<RoleSorter> sorters: [
        RoleSorter {
            property string tag: "intRole"
            property var expectedValues: [1, 2, 3, 4, 5]
            roleName: "intRole"
        },
        RoleSorter {
            property string tag: "intRoleDescending"
            property var expectedValues: [5, 4, 3, 2, 1]
            roleName: "intRole"
            sortOrder: Qt.DescendingOrder
        },
        RoleSorter {
            property string tag: "stringRole"
            property var expectedValues: ["a", "b", "c", "d", "e"]
            roleName: "stringRole"
        },
        RoleSorter {
            property string tag: "stringRoleDescending"
            property var expectedValues: ["e", "d", "c", "b", "a"]
            roleName: "stringRole"
            sortOrder: Qt.DescendingOrder
        },
        RoleSorter {
            property string tag: "mixedCaseStringRole"
            property var expectedValues: ["A", "b", "C", "D", "e"]
            roleName: "mixedCaseStringRole"
        }
    ]

    ListModel {
        id: dataModel
        ListElement { intRole: 5; stringRole: "c"; mixedCaseStringRole: "C" }
        ListElement { intRole: 3; stringRole: "e"; mixedCaseStringRole: "e" }
        ListElement { intRole: 1; stringRole: "d"; mixedCaseStringRole: "D" }
        ListElement { intRole: 2; stringRole: "a"; mixedCaseStringRole: "A" }
        ListElement { intRole: 4; stringRole: "b"; mixedCaseStringRole: "b" }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: dataModel
    }

    TestCase {
        name: "RoleSorterTests"

        function test_roleSorters_data() {
            return sorters;
        }

        function test_roleSorters(sorter) {
            testModel.sorters = sorter;

            verify(testModel.count === sorter.expectedValues.length,
                   "Expected count " + sorter.expectedValues.length + ", actual count: " + testModel.count);
            let actualValues = [];
            for (var i = 0; i < testModel.count; i++) {
                actualValues.push(testModel.get(i, sorter.roleName));
            }
            compare(actualValues, sorter.expectedValues);
        }
    }
}
