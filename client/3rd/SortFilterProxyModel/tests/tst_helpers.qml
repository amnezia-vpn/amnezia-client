import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import SortFilterProxyModel.Test 0.2

Item {
    ListModel {
        id: dataModel
        ListElement {
            firstName: "Tupac"
            lastName: "Shakur"
        }
        ListElement {
            firstName: "Charles"
            lastName: "Aznavour"
        }
        ListElement {
            firstName: "Frank"
            lastName: "Sinatra"
        }
        ListElement {
            firstName: "Laurent"
            lastName: "Garnier"
        }
        ListElement {
            firstName: "Phillipe"
            lastName: "Risoli"
        }
    }
    SortFilterProxyModel {
        id: testModel
        sourceModel: dataModel
    }
    SortFilterProxyModel {
        id: testModel2
        sourceModel: dataModel
        filters: ValueFilter {
            inverted: true
            roleName: "lastName"
            value: "Sinatra"
        }
        sorters: [
            RoleSorter { roleName: "lastName"},
            RoleSorter { roleName: "firstName"}
        ]
    }

    TestCase {
        name: "Helper functions"

        function test_getWithRoleName() {
            compare(testModel.get(0, "lastName"), "Shakur");
        }

        function test_getWithoutRoleName() {
            compare(testModel.get(1), { firstName: "Charles", lastName: "Aznavour"});
        }

        function test_roleForName() {
            compare(testModel.data(testModel.index(0, 0), testModel.roleForName("firstName")), "Tupac");
            compare(testModel.data(testModel.index(1, 0), testModel.roleForName("lastName")), "Aznavour");
        }

        function test_mapToSource() {
            compare(testModel2.mapToSource(3), 0);
            compare(testModel2.mapToSource(4), -1);
        }

        function test_mapToSourceLoop() {
            for (var i = 0; i < testModel2.count; ++i) {
                var sourceRow = testModel2.mapToSource(i);
                compare(testModel2.get(i).lastName, dataModel.get(sourceRow).lastName);
            }
        }

        function test_mapToSourceLoop_index() {
            for (var i = 0; i < testModel2.count; ++i) {
                var proxyIndex = testModel2.index(i, 0);
                var sourceIndex = testModel2.mapToSource(proxyIndex);
                var roleNumber = testModel2.roleForName("lastName");
                compare(testModel2.data(proxyIndex, roleNumber), dataModel.data(sourceIndex, roleNumber));
            }
        }

        function test_mapFromSource() {
            compare(testModel2.mapFromSource(1), 0);
            compare(testModel2.mapFromSource(2), -1);
        }

        function test_mapFromSourceLoop() {
            for (var i = 0; i < dataModel.count; ++i) {
                var proxyRow = testModel2.mapFromSource(i);
                if (proxyRow !== -1) {
                    compare(dataModel.get(i).lastName, testModel2.get(proxyRow).lastName);
                }
            }
        }

        function test_mapFromSourceLoop_index() {
            for (var i = 0; i < dataModel.count; ++i) {
                var sourceIndex = dataModel.index(i, 0);
                var proxyIndex = testModel2.mapFromSource(sourceIndex);
                var roleNumber = testModel2.roleForName("lastName");
                if (proxyIndex.valid)
                    compare(testModel2.data(proxyIndex, roleNumber), dataModel.data(sourceIndex, roleNumber));
            }
        }

    }
}
