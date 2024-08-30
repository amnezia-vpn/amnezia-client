import QtQuick
import QtQuick.Controls
import Qt.labs.platform

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
        // Fix calling paste from clipboard when launching app on android
        enabled: Qt.platform.os === "android" ? true : textObj.canPaste
        onTriggered: textObj.paste()
    }

    MenuItem {
        text: qsTr("&SelectAll")
        shortcut: StandardKey.SelectAll
        enabled: textObj.length > 0
        onTriggered: textObj.selectAll()
    }
}
