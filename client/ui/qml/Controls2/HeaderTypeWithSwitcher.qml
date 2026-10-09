import QtQuick
import QtQuick.Layouts

import Style 1.0

BaseHeaderType {
    id: root

    property var switcherFunction
    property bool showSwitcher: false
    property alias switcher: headerSwitcher

    Component.onCompleted: {
        headerRow.children.push(headerSwitcher)
    }

    SwitcherType {
        id: headerSwitcher
        Layout.alignment: Qt.AlignRight
        visible: root.showSwitcher
        accessibleName: root.headerText

        onToggled: {
            if (switcherFunction && typeof switcherFunction === "function") {
                switcherFunction(checked)
            }
        }
    }
} 
