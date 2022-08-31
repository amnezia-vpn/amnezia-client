import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import QtQml 2.2

Item {
    ListModel {
        id: listModel
        ListElement { firstName: "Justin"; lastName: "Timberlake" }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: listModel

        proxyRoles: JoinRole {
            id: joinRole
            name: "fullName"
            roleNames: ["firstName", "lastName"]
        }
    }

    Instantiator {
        id: instantiator
        model: testModel
        QtObject {
            property string fullName: model.fullName
        }
    }

    TestCase {
        name: "JoinRole"

        function test_joinRole() {
            compare(instantiator.object.fullName, "Justin Timberlake");
            listModel.setProperty(0, "lastName", "Bieber");
            compare(instantiator.object.fullName, "Justin Bieber");
            joinRole.roleNames = ["lastName", "firstName"];
            compare(instantiator.object.fullName, "Bieber Justin");
            joinRole.separator = " - ";
            compare(instantiator.object.fullName, "Bieber - Justin");
        }
    }
}
