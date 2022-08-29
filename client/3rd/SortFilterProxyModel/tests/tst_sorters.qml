import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import SortFilterProxyModel.Test 0.2

Item {
    ListModel {
        id: listModel
        ListElement { test: "first"; test2: "c"; test3: 1 }
        ListElement { test: "second"; test2: "a"; test3: 0 }
        ListElement { test: "third"; test2: "b"; test3: 2}
        ListElement { test: "fourth"; test2: "b"; test3: 3 }
    }

    ListModel {
        id: noRolesFirstListModel
    }

    property list<QtObject> sorters: [
        QtObject {
          property string tag: "no sorter"
          property bool notASorter: true
          property var expectedValues: ["first", "second", "third", "fourth"]
        },
        IndexSorter {
            property string tag: "Dummy IndexSorter"
            property var expectedValues: ["first", "second", "third", "fourth"]
        },
        ReverseIndexSorter {
            property string tag: "Dummy ReverseIndexSorter"
            property var expectedValues: ["fourth", "third", "second", "first"]
        },
        IndexSorter {
            property string tag: "Disabled dummy IndexSorter"
            enabled: false
            property var expectedValues: ["first", "second", "third", "fourth"]
        },
        ReverseIndexSorter {
            property string tag: "Disabled dummy ReverseIndexSorter"
            enabled: false
            property var expectedValues: ["first", "second", "third", "fourth"]
        },
        IndexSorter {
            property string tag: "Descending dummy IndexSorter"
            ascendingOrder: false
            property var expectedValues: ["fourth", "third", "second", "first"]
        },
        ReverseIndexSorter {
            property string tag: "Descending dummy ReverseIndexSorter"
            ascendingOrder: false
            property var expectedValues: ["first", "second", "third", "fourth"]
        },
        IndexSorter {
            property string tag: "Disabled descending dummy IndexSorter"
            enabled: false
            ascendingOrder: false
            property var expectedValues: ["first", "second", "third", "fourth"]
        },
        ReverseIndexSorter {
            property string tag: "Disabled descending dummy ReverseIndexSorter"
            enabled: false
            ascendingOrder: false
            property var expectedValues: ["first", "second", "third", "fourth"]
        }
    ]

    ReverseIndexSorter {
        id: reverseIndexSorter
    }

    property list<RoleSorter> tieSorters: [
        RoleSorter { roleName: "test2" },
        RoleSorter { roleName: "test" }
    ]

    property list<RoleSorter> sortersWithPriority: [
        RoleSorter { roleName: "test3" },
        RoleSorter { roleName: "test" },
        RoleSorter { roleName: "test2"; priority: 1 }
    ]

    SortFilterProxyModel {
        id: testModel
        sourceModel: listModel
    }

    SortFilterProxyModel {
        id: noRolesFirstProxyModel
        sourceModel: noRolesFirstListModel
        sorters: RoleSorter { roleName: "test" }
    }

    TestCase {
        name: "SortersTests"

        function test_indexOrder_data() {
            return sorters;
        }

        function test_indexOrder(sorter) {
            testModel.sorters = sorter;
            verifyModelValues(testModel, sorter.expectedValues);
        }

        function test_enablingSorter() {
            reverseIndexSorter.enabled = false;
            testModel.sorters = reverseIndexSorter;
            var expectedValuesBeforeEnabling = ["first", "second", "third", "fourth"];
            var expectedValuesAfterEnabling = ["fourth", "third", "second", "first"];
            verifyModelValues(testModel, expectedValuesBeforeEnabling);
            reverseIndexSorter.enabled = true;
            verifyModelValues(testModel, expectedValuesAfterEnabling);
        }

        function test_disablingSorter() {
            reverseIndexSorter.enabled = true;
            testModel.sorters = reverseIndexSorter;
            var expectedValuesBeforeDisabling = ["fourth", "third", "second", "first"];
            var expectedValuesAfterDisabling = ["first", "second", "third", "fourth"];
            verifyModelValues(testModel, expectedValuesBeforeDisabling);
            reverseIndexSorter.enabled = false;
            verifyModelValues(testModel, expectedValuesAfterDisabling);
        }

        function test_tieSorters() {
            testModel.sorters = tieSorters;
            var expectedValues = ["second", "fourth", "third", "first"];
            verifyModelValues(testModel, expectedValues);
        }

        function test_sortersWithPriority() {
            testModel.sorters = sortersWithPriority;
            var expectedValues = ["second", "third", "fourth", "first"];
            verifyModelValues(testModel, expectedValues);
            testModel.sorters[0].priority = 2;
            expectedValues = ["second", "first", "third", "fourth"];
            verifyModelValues(testModel, expectedValues);
        }

        function test_noRolesFirstModel() {
            noRolesFirstListModel.append([{test: "b"}, {test: "a"}]);
            var expectedValues = ["a", "b"];
            verifyModelValues(noRolesFirstProxyModel, expectedValues);
        }

        function verifyModelValues(model, expectedValues) {
            verify(model.count === expectedValues.length,
                   "Expected count " + expectedValues.length + ", actual count: " + model.count);
            for (var i = 0; i < model.count; i++)
            {
                var modelValue = model.get(i, "test");
                verify(modelValue === expectedValues[i],
                       "Expected testModel value " + expectedValues[i] + ", actual: " + modelValue);
            }
        }
    }
}
