import QtQuick 2.0
import SortFilterProxyModel 0.2
import QtQml.Models 2.2
import QtTest 1.1

Item {
    property list<RangeFilter> filters: [
        RangeFilter {
            property string tag: "inclusive"
            property int expectedModelCount: 3
            property var expectedValues: [3, 2, 4]
            property QtObject dataModel: dataModel0
            roleName: "value"; minimumValue: 2; maximumValue: 4
        },
        RangeFilter {
            property string tag: "explicitInclusive"
            property int expectedModelCount: 3
            property var expectedValues: [3, 2, 4]
            property QtObject dataModel: dataModel0
            roleName: "value"; minimumValue: 2; maximumValue: 4; minimumInclusive: true; maximumInclusive: true
        },
        RangeFilter {
            property string tag: "inclusiveMinExclusiveMax"
            property int expectedModelCount: 2
            property var expectedValues: [2, 3]
            property QtObject dataModel: dataModel1
            roleName: "value"; minimumValue: 2; maximumValue: 4; minimumInclusive: true; maximumInclusive: false
        },
        RangeFilter {
            property string tag: "exclusiveMinInclusiveMax"
            property int expectedModelCount: 2
            property var expectedValues: [3, 4]
            property QtObject dataModel: dataModel1
            roleName: "value"; minimumValue: 2; maximumValue: 4; minimumInclusive: false; maximumInclusive: true
        },
        RangeFilter {
            property string tag: "exclusive"
            property int expectedModelCount: 1
            property var expectedValues: [3]
            property QtObject dataModel: dataModel1
            roleName: "value"; minimumValue: 2; maximumValue: 4; minimumInclusive: false; maximumInclusive: false
        },
        RangeFilter {
            property string tag: "outOfBoundsRange"
            property var expectedValues: []
            property QtObject dataModel: dataModel1
            roleName: "value"; minimumValue: 4; maximumValue: 2
        },
        RangeFilter {
            objectName: tag
            property string tag: "noMinimum"
            property var expectedValues: [3, 1, 2]
            property QtObject dataModel: dataModel0
            roleName: "value"; maximumValue: 3
        }
    ]

    ListModel {
        id: dataModel0
        ListElement { value: 5 }
        ListElement { value: 3 }
        ListElement { value: 1 }
        ListElement { value: 2 }
        ListElement { value: 4 }
    }

    ListModel {
        id: dataModel1
        ListElement { value: 5 }
        ListElement { value: 2 }
        ListElement { value: 3 }
        ListElement { value: 1 }
        ListElement { value: 4 }
    }

    SortFilterProxyModel { id: testModel }

    TestCase {
        name:"RangeFilterTests"

        function test_minMax_data() {
            return filters;
        }

        function test_minMax(filter) {
            testModel.sourceModel = filter.dataModel;
            testModel.filters = filter;

            verify(testModel.count === filter.expectedValues.length,
                   "Expected count " + filter.expectedValues.length + ", actual count: " + testModel.count);
            for (var i = 0; i < testModel.count; i++)
            {
                var modelValue = testModel.get(i, filter.roleName);
                verify(modelValue === filter.expectedValues[i],
                       "Expected testModel value " + filter.expectedValues[i] + ", actual: " + modelValue);
            }
        }
    }
}
