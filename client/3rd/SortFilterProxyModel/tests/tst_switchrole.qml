import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import QtQml 2.2

Item {
    ListModel {
        id: listModel
        ListElement { name: "1"; favorite: true }
        ListElement { name: "2"; favorite: false }
        ListElement { name: "3"; favorite: false }
        ListElement { name: "4"; favorite: true }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: listModel

        proxyRoles: SwitchRole {
            id: switchRole
            name: "switchRole"
            ValueFilter {
                id: valueFilter
                roleName: "favorite"
                value: true
                SwitchRole.value: "*"
            }
            ValueFilter {
                id: secondValueFilter
                roleName: "favorite"
                value: true
                SwitchRole.value: "%"
            }
            ValueFilter {
                id: thirdValueFilter
                roleName: "name"
                value: 3
                SwitchRole.value: "three"
            }
            defaultRoleName: "name"
            defaultValue: "foo"
        }
    }

    Instantiator {
        id: instantiator
        model: testModel
        QtObject {
            property var switchRole: model.switchRole
        }
    }

    TestCase {
        name: "SwitchRole"

        function test_role() {
            compare(testModel.get(0, "switchRole"), "*");
            compare(testModel.get(1, "switchRole"), "2");
            compare(testModel.get(2, "switchRole"), "three");
            compare(testModel.get(3, "switchRole"), "*");
        }

        function test_valueChange() {
            compare(instantiator.object.switchRole, "*");
            valueFilter.SwitchRole.value = "test";
            compare(instantiator.object.switchRole, "test");
            valueFilter.SwitchRole.value = "*";
        }

        function test_filterChange() {
            compare(instantiator.object.switchRole, "*");
            valueFilter.enabled = false;
            compare(instantiator.object.switchRole, "%");
            valueFilter.enabled = true;
        }

        function test_defaultSourceChange() {
            compare(instantiator.object.switchRole, "*");
            listModel.setProperty(0, "favorite", false);
            compare(instantiator.object.switchRole, "1");
            compare(instantiator.objectAt(1).switchRole, "2");
            listModel.setProperty(1, "name", "test");
            compare(instantiator.objectAt(1).switchRole, "test");

            listModel.setProperty(1, "name", "2");
            listModel.setProperty(0, "favorite", true);
        }

        function test_defaultValue() {
            switchRole.defaultRoleName = "";
            compare(instantiator.objectAt(1).switchRole, "foo");
            switchRole.defaultValue = "bar";
            compare(instantiator.objectAt(1).switchRole, "bar");
            switchRole.defaultRoleName = "name";
            switchRole.defaultValue = "foo";
        }
    }
}
