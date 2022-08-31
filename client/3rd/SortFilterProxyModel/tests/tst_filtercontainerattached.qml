import QtQuick 2.0
import SortFilterProxyModel 0.2
import QtQml.Models 2.2
import QtQml 2.2
import QtTest 1.1

Item {

    ListModel {
        id: dataModel
        ListElement { a: 0; b: 0; c: 0 }
        ListElement { a: 0; b: 0; c: 1 }
        ListElement { a: 0; b: 1; c: 0 }
        ListElement { a: 0; b: 1; c: 1 }
        ListElement { a: 1; b: 0; c: 0 }
        ListElement { a: 1; b: 0; c: 1 }
        ListElement { a: 1; b: 1; c: 0 }
        ListElement { a: 1; b: 1; c: 1 }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: dataModel
    }

    Instantiator {
        id: filterInstantiator
        model: ["a", "b", "c"]
        delegate: ValueFilter {
            FilterContainer.container: testModel
            roleName: modelData
            value: 1
        }
    }

    TestCase {
        name: "FilterContainerAttached"

        function modelValues() {
            var modelValues = [];

            for (var i = 0; i < testModel.count; i++)
                modelValues.push(testModel.get(i));

            return modelValues;
        }

        function test_filterContainers() {
            compare(filterInstantiator.count, 3);
            compare(modelValues(), [ { a: 1, b: 1, c: 1 }]);
            filterInstantiator.model = ["a", "b"];
            wait(0);
            compare(filterInstantiator.count, 2)
            compare(modelValues(), [ { a: 1, b: 1, c: 0 }, { a: 1, b: 1, c: 1 }]);
        }
    }
}
