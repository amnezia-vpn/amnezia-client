import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Switch {
    id: root

    property alias descriptionText: description.text

    property string checkedIndicatorColor: "#412102"
    property string defaultIndicatorColor: "transparent"
    property string checkedIndicatorBorderColor: "#412102"
    property string defaultIndicatorBorderColor: "#494B50"

    property string checkedInnerCircleColor: "#FBB26A"
    property string defaultInnerCircleColor: "#D7D8DB"

    property string hoveredIndicatorBackgroundColor: Qt.rgba(1, 1, 1, 0.08)
    property string defaultIndicatorBackgroundColor: "transparent"

    implicitWidth: content.implicitWidth + switcher.implicitWidth
    implicitHeight: content.implicitHeight

    indicator: Rectangle {
        id: switcher

        anchors.left: content.right
        anchors.verticalCenter: parent.verticalCenter

        implicitWidth: 52
        implicitHeight: 32

        radius: 16
        color: root.checked ? checkedIndicatorColor : defaultIndicatorColor
        border.color: root.checked ? checkedIndicatorBorderColor : defaultIndicatorBorderColor

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
        Behavior on border.color {
            PropertyAnimation { duration: 200 }
        }

        Rectangle {
            id: innerCircle

            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? parent.width - width - 4 : 8
            width: root.checked ? 24 : 16
            height: root.checked ? 24 : 16
            radius: 23
            color: root.checked ? checkedInnerCircleColor : defaultInnerCircleColor

            Behavior on x {
                PropertyAnimation { duration: 200 }
            }
        }

        Rectangle {
            anchors.centerIn: innerCircle
            width: 40
            height: 40
            radius: 23
            color: hovered ? hoveredIndicatorBackgroundColor : defaultIndicatorBackgroundColor

            Behavior on color {
                PropertyAnimation { duration: 200 }
            }
        }
    }

    contentItem: ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.rightMargin: switcher.implicitWidth

        ListItemTitleType {
            Layout.fillWidth: true

            text: root.text
        }

        CaptionTextType {
            id: description

            Layout.fillWidth: true

            color: "#878B91"

            visible: text !== ""
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }
}
