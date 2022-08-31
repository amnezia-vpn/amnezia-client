import QtQuick 2.0
import SortFilterProxyModel 0.2
import QtQml.Models 2.2
import QtTest 1.1

Item {
    property list<IndexFilter> filters: [
        IndexFilter {
            property string tag: "basicUsage"
            property var expectedValues: [3, 1, 2]
            minimumIndex: 1; maximumIndex: 3
        },
        IndexFilter {
            property string tag: "outOfBounds"
            property var expectedValues: []
            minimumIndex: 3; maximumIndex: 1
        },
        IndexFilter {
            property string tag: "0to0Inverted"
            property var expectedValues: [3,1,2,4]
            minimumIndex: 0; maximumIndex: 0; inverted: true
        },
        IndexFilter {
            property string tag: "0to0" // bug / issue #15
            property var expectedValues: [5]
            minimumIndex: 0; maximumIndex: 0
        },
        IndexFilter {
            property string tag: "basicUsageInverted"
            property var expectedValues: [5,4]
            minimumIndex: 1; maximumIndex: 3; inverted: true
        },
        IndexFilter {
            property string tag: "last"
            property var expectedValues: [4]
            minimumIndex: -1
        },
        IndexFilter {
            property string tag: "fromEnd"
            property var expectedValues: [2, 4]
            minimumIndex: -2
        },
        IndexFilter {
            property string tag: "fromEndRange"
            property var expectedValues: [1, 2]
            minimumIndex: -3
            maximumIndex: -2
        },
        IndexFilter {
            property string tag: "mixedSignRange"
            property var expectedValues: [3, 1, 2]
            minimumIndex: 1
            maximumIndex: -2
        },
        IndexFilter {
            property string tag: "toBigFilter"
            property var expectedValues: []
            minimumIndex: 5
        },
        IndexFilter {
            property string tag: "noFilter"
            property var expectedValues: [5, 3, 1, 2, 4]
        },
        IndexFilter {
            property string tag: "undefinedFilter"
            property var expectedValues: [5, 3, 1, 2, 4]
            minimumIndex: undefined
            maximumIndex: null
        }
    ]

    ListModel {
        id: dataModel
        ListElement { value: 5 }
        ListElement { value: 3 }
        ListElement { value: 1 }
        ListElement { value: 2 }
        ListElement { value: 4 }
    }

    SortFilterProxyModel {
        id: testModel
        // FIXME: Crashes/fails with error if I define ListModel directly within sourceModel
        sourceModel: dataModel
    }

    TestCase {
        name: "IndexFilterTests"

        function test_minMax_data() {
            return filters;
        }

        function test_minMax(filter) {
            testModel.filters = filter;

            var actualValues = [];
            for (var i = 0; i < testModel.count; i++)
                actualValues.push(testModel.data(testModel.index(i, 0)));

            compare(actualValues, filter.expectedValues);
        }
    }
}
