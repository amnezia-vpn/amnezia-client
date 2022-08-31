import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import SortFilterProxyModel.Test 0.2

Item {
    ListModel {
        id: testModel1
        ListElement{ role1: 1 }
    }
    SortFilterProxyModel {
        id: testFilterProxyModel
        sourceModel: testModel1
        property int foo: 1
        filters: [
            ExpressionFilter {
                id: expressionFilter
                property var w: ({count : 0}) // wrap count in a js object so modifying it doesn't bind it in the expression
                expression: {
                    ++w.count;
                    testFilterProxyModel.foo;
                    return true;
                }
            },
            ValueFilter {
                roleName: "role1"
                value: testFilterProxyModel.foo
            },
            ValueFilter {
                roleName: "role1"
                value: testFilterProxyModel.foo
            }
        ]
        sorters: RoleSorter {
            roleName: "role1"
            sortOrder: testFilterProxyModel.foo === 1 ? Qt.AscendingOrder : Qt.DescendingOrder
        }
    }

    ListModel {
        id: testModel2
        ListElement{ role1: 1 }
        ListElement{ role1: 2 }
    }
    SortFilterProxyModel {
        id: testSorterProxyModel
        sourceModel: testModel2
        property bool foo: true
        sorters: [
            ExpressionSorter {
                id: expressionSorter
                property var w: ({count : 0}) // wrap count in a js object so modifying it doesn't bind it in the expression
                expression: {
                    ++w.count;
                    testSorterProxyModel.foo;
                    return false;
                }
            },
            RoleSorter {
                roleName: "role1"
                sortOrder: testSorterProxyModel.foo ? Qt.AscendingOrder : Qt.DescendingOrder
            },
            RoleSorter {
                roleName: "role1"
                sortOrder: testSorterProxyModel.foo ? Qt.AscendingOrder : Qt.DescendingOrder
            }
        ]
    }

    SortFilterProxyModel {
        id: testRolesProxyModel
        sourceModel: testModel1
        property bool foo: true
        proxyRoles: [
            StaticRole {
                name: "display"
                value: 5
            },
            ExpressionRole {
                id: expressionRole
                name: "expressionRole"
                property var w: ({count : 0}) // wrap count in a js object so modifying it doesn't bind it in the expression
                expression: {
                    ++w.count;
                    return testRolesProxyModel.foo;
                }
            },
            StaticRole {
                name: "role1"
                value: testRolesProxyModel.foo
            },
            StaticRole {
                name: "role2"
                value: testRolesProxyModel.foo
            }
        ]
    }

    SignalSpy {
        id: dataChangedSpy
        target: testRolesProxyModel
        signalName: "dataChanged"
    }

    Instantiator {
        id: instantiator
        model: testRolesProxyModel
        delegate: QtObject { property bool foo: model.expressionRole; property bool foo2: model.expressionRole }
    }

    TestCase {
        name: "DelayedTest"

        function test_directFilters() {
            testFilterProxyModel.delayed = false;
            expressionFilter.w.count = 0;
            testFilterProxyModel.foo = 2;
            compare(testFilterProxyModel.count, 0);
            verify(expressionFilter.w.count > 1);
            var lastEvaluationCount = expressionFilter.w.count;
            wait(0);
            compare(testFilterProxyModel.count, 0);
            compare(expressionFilter.w.count, lastEvaluationCount);
        }

        function test_delayedFilters() {
            testFilterProxyModel.delayed = false;
            testFilterProxyModel.foo = 2;
            compare(testFilterProxyModel.count, 0);
            testFilterProxyModel.delayed = true;
            expressionFilter.w.count = 0;
            testFilterProxyModel.foo = 0;
            testFilterProxyModel.foo = 1;
            compare(testFilterProxyModel.count, 0);
            compare(expressionFilter.w.count, 0);
            wait(0);
            compare(testFilterProxyModel.count, 1);
            compare(expressionFilter.w.count, 1);
        }

        function test_directSorters() {
            testSorterProxyModel.delayed = false;
            testSorterProxyModel.foo = true;
            compare(testSorterProxyModel.get(0).role1, 1);
            expressionSorter.w.count = 0;
            testSorterProxyModel.foo = false;
            compare(testSorterProxyModel.get(0).role1, 2);
            verify(expressionSorter.w.count > 1);
            var lastEvaluationCount = expressionSorter.w.count
            wait(0);
            compare(testSorterProxyModel.get(0).role1, 2);
            compare(expressionSorter.w.count, lastEvaluationCount);
        }

        function test_delayedSorters() {
            testSorterProxyModel.delayed = false;
            testSorterProxyModel.foo = true;
            compare(testSorterProxyModel.get(0).role1, 1);
            testSorterProxyModel.delayed = true;
            expressionSorter.w.count = 0;
            testSorterProxyModel.foo = false;
            testSorterProxyModel.foo = true;
            testSorterProxyModel.foo = false;
            compare(testSorterProxyModel.get(0).role1, 1);
            compare(expressionSorter.w.count, 0);
            wait(0);
            compare(testSorterProxyModel.get(0).role1, 2);
            compare(expressionSorter.w.count, 2);
        }

        function test_proxyRoles() {
            // init not delayed
            testRolesProxyModel.delayed = false;
            testRolesProxyModel.foo = true;
            compare(instantiator.object.foo, true);
            expressionRole.w.count = 0;
            dataChangedSpy.clear();

            // test not delayed
            testRolesProxyModel.foo = false;
            compare(instantiator.object.foo, false);
            compare(dataChangedSpy.count, 3);
            var notDelayedCount = expressionRole.w.count; // why is it 12 and not just 3 ?
            wait(0);
            compare(instantiator.object.foo, false);
            compare(dataChangedSpy.count, 3);
            compare(expressionRole.w.count, notDelayedCount);

            // init delayed
            testRolesProxyModel.delayed = true;
            expressionRole.w.count = 0;
            dataChangedSpy.clear();

            // test delayed
            testRolesProxyModel.foo = true;
            testRolesProxyModel.foo = false;
            testRolesProxyModel.foo = true;
            compare(instantiator.object.foo, false);
            compare(dataChangedSpy.count, 0);
            compare(expressionRole.w.count, 0);
            wait(0);
            compare(instantiator.object.foo, true);
            compare(dataChangedSpy.count, 1);
            var expectedDelayedCount = notDelayedCount / 3;
            compare(expressionRole.w.count, expectedDelayedCount);
        }
    }
}
