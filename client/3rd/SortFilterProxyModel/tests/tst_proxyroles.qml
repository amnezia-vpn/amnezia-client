import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import SortFilterProxyModel.Test 0.2
import QtQml 2.2

Item {
    ListModel {
        id: listModel
        ListElement { test: "first"; keep: true }
        ListElement { test: "second"; keep: true }
        ListElement { test: "third"; keep: true }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: listModel
        filters: [
            ValueFilter {
                roleName: "keep"
                value: true
            },
            ValueFilter {
                inverted: true
                roleName: "staticRole"
                value: "filterMe"
            }
        ]

        proxyRoles: [
            StaticRole {
                id: staticRole
                name: "staticRole"
                value: "foo"
            },
            StaticRole {
                id: renameRole
                name: "renameMe"
                value: "test"
            },
            SourceIndexRole {
                name: "sourceIndexRole"
            },
            MultiRole {}
        ]
    }

    Instantiator {
        id: instantiator
        model: testModel
        QtObject {
            property string staticRole: model.staticRole
            property int sourceIndexRole: model.sourceIndexRole
        }
    }

    ListModel {
        id: singleRowModel
        ListElement {
            changingRole: "Change me"
            otherRole: "I don't change"
        }
    }

    SortFilterProxyModel {
        id: noProxyRolesProxyModel
        sourceModel: singleRowModel
    }

    Instantiator {
        id: outerInstantiator
        model: noProxyRolesProxyModel
        QtObject {
            property var counter: ({ count : 0 })
            property string changingRole: model.changingRole
            property string otherRole: {
                ++counter.count;
                return model.otherRole;
            }
        }
    }

    TestCase {
        name: "ProxyRoles"

        function test_resetAfterNameChange() {
            var oldObject = instantiator.object;
            renameRole.name = "foobarRole";
            var newObject = instantiator.object;
            verify(newObject !== oldObject, "Instantiator object should have been reinstantiated");
        }

        function test_proxyRoleInvalidation() {
            compare(instantiator.object.staticRole, "foo");
            staticRole.value = "bar";
            compare(instantiator.object.staticRole, "bar");
        }

        function test_proxyRoleGetDataFromSource() {
            compare(instantiator.object.sourceIndexRole, 0);
            compare(testModel.get(1, "sourceIndexRole"), 1);
            listModel.setProperty(1, "keep", false);
            compare(testModel.get(1, "sourceIndexRole"), 2);
        }

        function test_filterFromProxyRole() {
            staticRole.value = "filterMe";
            compare(testModel.count, 0);
            staticRole.value = "foo";
            compare(testModel.count, 3);
        }

        function test_multiRole() {
            compare(testModel.get(0, "role1"), "data for role1");
            compare(testModel.get(0, "role2"), "data for role2");
        }

        function test_ProxyRolesDataChanged() {
            outerInstantiator.object.counter.count = 0;
            singleRowModel.setProperty(0, "changingRole", "Changed")
            compare(outerInstantiator.object.changingRole, "Changed");
            compare(outerInstantiator.object.counter.count, 0);
        }
    }
}
