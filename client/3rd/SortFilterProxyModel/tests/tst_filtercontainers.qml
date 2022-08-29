import QtQuick 2.0
import SortFilterProxyModel 0.2
import QtQml.Models 2.2
import QtTest 1.1

Item {
    property list<Filter> filters: [
        AllOf {
            property string tag: "allOf"
            property var expectedValues: [{a: 0, b: false}]
            ValueFilter {
                roleName: "a"
                value: "0"
            }
            ValueFilter {
                roleName: "b"
                value: false
            }
        },
        AllOf {
            property string tag: "allOfOneDisabled"
            property var expectedValues: [{a: 0, b: true}, {a: 0, b: false}]
            ValueFilter {
                roleName: "a"
                value: "0"
            }
            ValueFilter {
                enabled: false
                roleName: "b"
                value: false
            }
        },
        AnyOf {
            property string tag: "anyOf"
            property var expectedValues: [{a: 0, b: true}, {a: 0, b: false}, {a: 1, b: false}]
            ValueFilter {
                roleName: "a"
                value: "0"
            }
            ValueFilter {
                roleName: "b"
                value: false
            }
        }
    ]

    AllOf {
        id: outerFilter
        ValueFilter {
            roleName: "a"
            value: "0"
        }
        ValueFilter {
            id: innerFilter
            roleName: "b"
            value: false
        }
    }

    ListModel {
        id: dataModel
        ListElement { a: 0; b: true }
        ListElement { a: 0; b: false }
        ListElement { a: 1; b: true }
        ListElement { a: 1; b: false }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: dataModel
    }

    TestCase {
        name:"RangeFilterTests"

        function modelValues() {
            var modelValues = [];

            for (var i = 0; i < testModel.count; i++)
                modelValues.push(testModel.get(i));

            return modelValues;
        }

        function test_filterContainers_data() {
            return filters;
        }

        function test_filterContainers(filter) {
            testModel.filters = filter;
            compare(JSON.stringify(modelValues()), JSON.stringify(filter.expectedValues));
        }

        function test_changeInnerFilter() {
            testModel.filters = outerFilter;
            compare(JSON.stringify(modelValues()), JSON.stringify([{a: 0, b: false}]));
            innerFilter.value = true;
            compare(JSON.stringify(modelValues()), JSON.stringify([{a: 0, b: true}]));
            innerFilter.enabled = false;
            compare(JSON.stringify(modelValues()), JSON.stringify([{a: 0, b: true}, {a: 0, b: false}]));
        }
    }
}
