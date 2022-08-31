import QtQuick 2.0
import QtTest 1.1
import QtQml 2.2
import SortFilterProxyModel 0.2

Item {
    ListModel {
        id: nonEmptyFirstModel
        ListElement {
            test: "test"
        }
    }
    SortFilterProxyModel {
        id: nonEmptyFirstProxyModel
        sourceModel: nonEmptyFirstModel
    }
    Instantiator {
        id: nonEmptyFirstInstantiator
        model: nonEmptyFirstProxyModel
        QtObject { property var test: model.test }
    }

    ListModel {
        id: emptyFirstModel
    }
    SortFilterProxyModel {
        id: emptyFirstProxyModel
        sourceModel: emptyFirstModel
    }
    Instantiator {
        id: emptyFirstInstantiator
        model: emptyFirstProxyModel
        QtObject { property var test: model.test }
    }

    TestCase {
        name: "RoleTests"

        function test_nonEmptyFirst() {
            compare(nonEmptyFirstInstantiator.object.test, "test");
        }

        function test_emptyFirst() {
            emptyFirstModel.append({test: "test"});
            compare(emptyFirstProxyModel.get(0), {test: "test"});
            compare(emptyFirstInstantiator.object.test, "test");
        }
    }
}
