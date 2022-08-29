import QtQuick 2.0
import SortFilterProxyModel 0.2
import QtQml.Models 2.2
import QtQml 2.2
import QtTest 1.1

Item {

    ListModel {
        id: dataModel
        ListElement { a: 3; b: 2; c: 9 }
        ListElement { a: 3; b: 5; c: 0 }
        ListElement { a: 3; b: 2; c: 8 }
        ListElement { a: 2; b: 9; c: 1 }
        ListElement { a: 2; b: 1; c: 7 }
        ListElement { a: 2; b: 6; c: 2 }
        ListElement { a: 1; b: 8; c: 6 }
        ListElement { a: 1; b: 7; c: 3 }
        ListElement { a: 1; b: 8; c: 5 }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: dataModel
    }
    ListModel {
        id: sorterRoleModel
        ListElement { roleName: "a" }
        ListElement { roleName: "b" }
        ListElement { roleName: "c" }
    }
    Instantiator {
        id: sorterInstantiator
        model: sorterRoleModel
        delegate: RoleSorter {
            SorterContainer.container: testModel
            roleName: model.roleName
        }
    }

    SortFilterProxyModel {
        id: testModelPriority
        sourceModel: dataModel
    }
    ListModel {
        id: sorterRoleModelPriority
        ListElement { roleName: "a" }
        ListElement { roleName: "b" }
        ListElement { roleName: "c" }
    }
    Instantiator {
        id: sorterInstantiatorPriority
        model: sorterRoleModelPriority
        delegate: RoleSorter {
            SorterContainer.container: testModelPriority
            roleName: model.roleName
            priority: -model.index
        }
    }

    TestCase {
        name: "SorterContainerAttached"

        function modelValues(model) {
            var modelValues = [];

            for (var i = 0; i < model.count; i++)
                modelValues.push(model.get(i));

            return modelValues;
        }

        function test_sorterContainers() {
            compare(sorterInstantiator.count, 3);
            compare(modelValues(testModel), [
                        { a: 1, b: 7, c: 3 },
                        { a: 1, b: 8, c: 5 },
                        { a: 1, b: 8, c: 6 },
                        { a: 2, b: 1, c: 7 },
                        { a: 2, b: 6, c: 2 },
                        { a: 2, b: 9, c: 1 },
                        { a: 3, b: 2, c: 8 },
                        { a: 3, b: 2, c: 9 },
                        { a: 3, b: 5, c: 0 }
                    ]);
            sorterRoleModel.remove(0); // a, b, c --> b, c
            wait(0);
            compare(sorterInstantiator.count, 2);
            compare(JSON.stringify(modelValues(testModel)), JSON.stringify([
                        { a: 2, b: 1, c: 7 },
                        { a: 3, b: 2, c: 8 },
                        { a: 3, b: 2, c: 9 },
                        { a: 3, b: 5, c: 0 },
                        { a: 2, b: 6, c: 2 },
                        { a: 1, b: 7, c: 3 },
                        { a: 1, b: 8, c: 5 },
                        { a: 1, b: 8, c: 6 },
                        { a: 2, b: 9, c: 1 },
                    ]));
        }

        function test_sorterContainersPriority() {
            compare(sorterInstantiatorPriority.count, 3);
            compare(JSON.stringify(modelValues(testModelPriority)), JSON.stringify([
                        { a: 1, b: 7, c: 3 },
                        { a: 1, b: 8, c: 5 },
                        { a: 1, b: 8, c: 6 },
                        { a: 2, b: 1, c: 7 },
                        { a: 2, b: 6, c: 2 },
                        { a: 2, b: 9, c: 1 },
                        { a: 3, b: 2, c: 8 },
                        { a: 3, b: 2, c: 9 },
                        { a: 3, b: 5, c: 0 }
                    ]));
            sorterRoleModelPriority.move(0, 1, 1); // a, b, c --> b, a, c
            wait(0);
            compare(sorterInstantiatorPriority.count, 3);
            compare(JSON.stringify(modelValues(testModelPriority)), JSON.stringify([
                        { a: 2, b: 1, c: 7 },
                        { a: 3, b: 2, c: 8 },
                        { a: 3, b: 2, c: 9 },
                        { a: 3, b: 5, c: 0 },
                        { a: 2, b: 6, c: 2 },
                        { a: 1, b: 7, c: 3 },
                        { a: 1, b: 8, c: 5 },
                        { a: 1, b: 8, c: 6 },
                        { a: 2, b: 9, c: 1 }
                    ]));
        }
    }
}
