import QtQuick
import QtQuick.Controls

TabButton {
    id: root

    property string hoveredColor: "#633303"
    property string defaultColor: "#D7D8DB"
    property string selectedColor: "#FBB26A"

    property string image

    property bool isSelected: false

    property string borderFocusedColor: "#D7D8DB"
    property int borderFocusedWidth: 1

    property var clickedFunc

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    icon.source: image
    icon.color: isSelected ? selectedColor : defaultColor

    background: Rectangle {
        id: background
        anchors.fill: parent
        color: "transparent"
        radius: 10

        border.color: root.activeFocus ? root.borderFocusedColor : "transparent"
        border.width: root.activeFocus ? root.borderFocusedWidth : 0

    }

    MouseArea {
        anchors.fill: background
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }

    Keys.onEnterPressed: {
        if (root.clickedFunc && typeof root.clickedFunc === "function") {
            root.clickedFunc()
        }
    }

    Keys.onReturnPressed: {
        if (root.clickedFunc && typeof root.clickedFunc === "function") {
            root.clickedFunc()
        }
    }

    onClicked: {
        if (root.clickedFunc && typeof root.clickedFunc === "function") {
            root.clickedFunc()
        }
    }
}
