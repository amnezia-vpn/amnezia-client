import QtQuick 2.0
import QtQml 2.2
import QtTest 1.1
import SortFilterProxyModel 0.2
import QtQml 2.2

Item {
    ListModel {
        id: listModel
        ListElement { dummyRole: false; compoundRole: "0 - zero"; unusedRole: "" }
        ListElement { dummyRole: false; compoundRole: "1 - one"; unusedRole: "" }
        ListElement { dummyRole: false; compoundRole: "2 - two"; unusedRole: "" }
        ListElement { dummyRole: false; compoundRole: "3 - three"; unusedRole: "" }
        ListElement { dummyRole: false; compoundRole: "four"; unusedRole: "" }
    }

    SortFilterProxyModel {
        id: testModel
        sourceModel: listModel

        proxyRoles: [
            RegExpRole {
                id: regExpRole
                roleName: "compoundRole"
                pattern: "(?<id>\\d+) - (?<name>.+)"
            },
            RegExpRole {
                id: caseSensitiveRole
                roleName: "compoundRole"
                pattern: "\\d+ - (?<nameCS>[A-Z]+)"
                caseSensitivity: Qt.CaseSensitive
            },
            RegExpRole {
                id: caseInsensitiveRole
                roleName: "compoundRole"
                pattern: "\\d+ - (?<nameCIS>[A-Z]+)"
                caseSensitivity: Qt.CaseInsensitive
            }
        ]
    }

            TestCase {
                name: "RegExpRole"

                function test_regExpRole() {
                    compare(testModel.get(0, "id"), "0");
                    compare(testModel.get(1, "id"), "1");
                    compare(testModel.get(0, "name"), "zero");
                    compare(testModel.get(4, "id"), undefined);
                    compare(testModel.get(0, "nameCS"), undefined);
                    compare(testModel.get(0, "nameCIS"), "zero");
                }
            }
    }
