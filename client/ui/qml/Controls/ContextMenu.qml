import QtQuick 2.12
import QtQuick.Controls 2.12
import Qt.labs.platform 1.0

Menu {
    property var textObj

    MenuItem {
        text: qsTr("C&ut")
        shortcut: StandardKey.Cut
        enabled: textObj.selectedText
        onTriggered: textObj.cut()
    }
    MenuItem {
        text: qsTr("&Copy")
        shortcut: StandardKey.Copy
        enabled: textObj.selectedText
        onTriggered: textObj.copy()
    }
    MenuItem {
        text: qsTr("&Paste")
        shortcut: StandardKey.Paste
        enabled: textObj.canPaste
        onTriggered: textObj.paste()
    }

    MenuItem {
        text: qsTr("&SelectAll")
        shortcut: StandardKey.SelectAll
        enabled: textObj.length > 0
        onTriggered: textObj.selectAll()
    }
}
