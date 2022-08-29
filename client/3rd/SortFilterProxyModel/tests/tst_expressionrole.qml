import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import QtQml 2.2

Item {
    property int c: 0
    ListModel {
        id: listModel
        ListElement { a: 1; b: 2 }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: listModel

        proxyRoles: ExpressionRole {
            name: "expressionRole"
            expression: a + model.b + c
        }
    }

    Instantiator {
        id: instantiator
        model: testModel
        QtObject {
            property string expressionRole: model.expressionRole
        }
    }

    TestCase {
        name: "ExpressionRole"

        function test_expressionRole() {
            fuzzyCompare(instantiator.object.expressionRole, 3, 1e-7);
            listModel.setProperty(0, "b", 9);
            fuzzyCompare(instantiator.object.expressionRole, 10, 1e-7);
            c = 1327;
            fuzzyCompare(instantiator.object.expressionRole, 1337, 1e-7);
        }
    }
}
