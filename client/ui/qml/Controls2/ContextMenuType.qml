import QtQuick
import QtQuick.Controls

Menu {
    property var textObj

    popupType: Popup.Native

    onAboutToShow: blocker.enabled = true
    onClosed: blocker.enabled = false

    MenuItem {
        text: qsTr("C&ut")
        enabled: textObj.selectedText
        onTriggered: textObj.cut()
    }
    MenuItem {
        text: qsTr("&Copy")
        enabled: textObj.selectedText
        onTriggered: textObj.copy()
    }
    MenuItem {
        text: qsTr("&Paste")
        // Fix calling paste from clipboard when launching app on android
        enabled: Qt.platform.os === "android" ? true : textObj.canPaste
        onTriggered: textObj.paste()
    }

    MenuItem {
        text: qsTr("&SelectAll")
        enabled: textObj.length > 0
        onTriggered: textObj.selectAll()
    }

    MouseArea {
        id: blocker
        z: 2
        enabled: false
        preventStealing: true
    }
}
