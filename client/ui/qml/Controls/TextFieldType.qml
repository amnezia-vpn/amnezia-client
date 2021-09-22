import QtQuick 2.12
import QtQuick.Controls 2.12
import Qt.labs.platform 1.0

TextField {
    id: root
    property bool error: false

    width: parent.width - 80
    height: 40
    anchors.topMargin: 5
    selectByMouse: true

    selectionColor: "darkgray"
    font.pixelSize: 16
    color: "#333333"
    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 40
        border.width: 1
        color: {
            if (root.error) {
                return Qt.rgba(213, 40, 60, 255)
            }
            return root.enabled ? "#F4F4F4" : Qt.rgba(127, 127, 127, 255)
        }
        border.color: {
            if (!root.enabled) {
                return Qt.rgba(127, 127, 127, 255)
            }
            if (root.error) {
                return Qt.rgba(213, 40, 60, 255)
            }
            if (root.focus) {
                return "#A7A7A7"
            }
            return "#A7A7A7"
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: contextMenu.open()
    }

    Menu {
        id: contextMenu

        onAboutToShow: console.log("aboutToShow")
        onAboutToHide: console.log("aboutToHide")

        MenuItem {
            text: qsTr("C&ut")
            shortcut: StandardKey.Cut
            enabled: root.selectedText
            onTriggered: root.cut()
        }
        MenuItem {
            text: qsTr("&Copy")
            shortcut: StandardKey.Copy
            enabled: root.selectedText
            onTriggered: root.copy()
        }
        MenuItem {
            text: qsTr("&Paste")
            shortcut: StandardKey.Paste
            enabled: root.canPaste
            onTriggered: root.paste()
        }
    }
}
