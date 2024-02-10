import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Switch {
    id: root

    property alias descriptionText: description.text
    property string descriptionTextColor: "#878B91"
    property string descriptionTextDisabledColor: "#494B50"

    property string textColor:  "#D7D8DB"
    property string textDisabledColor: "#878B91"

    property string checkedIndicatorColor: "#633303"
    property string defaultIndicatorColor: "transparent"
    property string checkedDisabledIndicatorColor: "#402102"

    property string checkedIndicatorBorderColor: "#633303"
    property string defaultIndicatorBorderColor: "#494B50"
    property string checkedDisabledIndicatorBorderColor: "#402102"

    property string checkedInnerCircleColor: "#FBB26A"
    property string defaultInnerCircleColor: "#D7D8DB"
    property string checkedDisabledInnerCircleColor: "#84603D"
    property string defaultDisabledInnerCircleColor: "#494B50"

    property string hoveredIndicatorBackgroundColor: Qt.rgba(1, 1, 1, 0.08)
    property string defaultIndicatorBackgroundColor: "transparent"

    hoverEnabled: enabled ? true : false

    indicator: Rectangle {
        id: switcher

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        implicitWidth: 52
        implicitHeight: 32

        radius: 16
        color: root.checked ? (root.enabled ? root.checkedIndicatorColor : root.checkedDisabledIndicatorColor)
                            : root.defaultIndicatorColor
        border.color: root.checked ? (root.enabled ? root.checkedIndicatorBorderColor : root.checkedDisabledIndicatorBorderColor)
                                   : root.defaultIndicatorBorderColor

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
            color: root.checked ? (root.enabled ? root.checkedInnerCircleColor : root.checkedDisabledInnerCircleColor)
                                : (root.enabled ? root.defaultInnerCircleColor : root.defaultDisabledInnerCircleColor)

            Behavior on x {
                PropertyAnimation { duration: 200 }
            }
        }

        Rectangle {
            anchors.centerIn: innerCircle
            width: 40
            height: 40
            radius: 23
            color: root.hovered ? root.hoveredIndicatorBackgroundColor : root.defaultIndicatorBackgroundColor

            Behavior on color {
                PropertyAnimation { duration: 200 }
            }
        }
    }

    contentItem: ColumnLayout {
        id: content

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left

        ListItemTitleType {
            Layout.fillWidth: true
            rightPadding: indicator.width

            text: root.text
            color: root.enabled ? root.textColor : root.textDisabledColor
        }

        CaptionTextType {
            id: description

            Layout.fillWidth: true
            rightPadding: indicator.width

            color: root.enabled ? root.descriptionTextColor : root.descriptionTextDisabledColor

            visible: text !== ""
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }
}
