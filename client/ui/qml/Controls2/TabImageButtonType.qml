import QtQuick
import QtQuick.Controls

import Style 1.0

TabButton {
    id: root

    property string hoveredColor: AmneziaStyle.color.brown
    property string defaultColor: AmneziaStyle.color.white
    property string selectedColor: AmneziaStyle.color.orange

    property string image

    property bool isSelected: false

    property string borderFocusedColor: AmneziaStyle.color.white
    property int borderFocusedWidth: 1

    property var clickedFunc

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    icon.source: image
    icon.color: isSelected ? selectedColor : defaultColor

    background: Rectangle {
        id: background
        anchors.fill: parent
        color: AmneziaStyle.color.transparent
        radius: 10

        border.color: root.activeFocus ? root.borderFocusedColor : AmneziaStyle.color.transparent
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
