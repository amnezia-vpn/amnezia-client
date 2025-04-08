import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Config 1.0

import "TextTypes"

Switch {
    id: root

    property string checkedIndicatorColor: Style.color.white
    property string defaultIndicatorColor: Style.color.transparent
    property string checkedDisabledIndicatorColor: Style.color.white

    property string borderFocusedColor: Style.color.gray3
    property int borderFocusedWidth: 1

    property string checkedIndicatorBorderColor: Style.color.gray3
    property string defaultIndicatorBorderColor: Style.color.gray3
    property string checkedDisabledIndicatorBorderColor: Style.color.gray3

    property string checkedInnerCircleColor: Style.color.accent1
    property string defaultInnerCircleColor: Style.color.black
    property string checkedDisabledInnerCircleColor: Style.color.accent1
    property string defaultDisabledInnerCircleColor: Style.color.black

    property string hoveredIndicatorBackgroundColor: Style.color.fivePercentBlack
    property string defaultIndicatorBackgroundColor: Style.color.transparent

    indicator: Rectangle {
        id: switcher

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        implicitWidth: 52
        implicitHeight: 32

        radius: 16
        color: root.checked ? (root.enabled ? root.checkedIndicatorColor : root.checkedDisabledIndicatorColor)
                            : root.defaultIndicatorColor

        border.color: root.activeFocus ? root.borderFocusedColor : (root.checked ? (root.enabled ? root.checkedIndicatorBorderColor : root.checkedDisabledIndicatorBorderColor)
                            : root.defaultIndicatorBorderColor)

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

    // contentItem: ColumnLayout {
    //     id: content

    //     anchors.verticalCenter: parent.verticalCenter
    //     anchors.left: parent.left

    //     ListItemTitleType {
    //         Layout.fillWidth: true
    //         rightPadding: indicator.width

    //         text: root.text
    //         color: root.enabled ? root.textColor : root.textDisabledColor
    //     }

    //     CaptionTextType {
    //         id: description

    //         Layout.fillWidth: true
    //         rightPadding: indicator.width

    //         color: root.enabled ? root.descriptionTextColor : root.descriptionTextDisabledColor

    //         visible: text !== ""
    //     }
    // }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }

    function handleSwitch(event) {
        if (!event.isAutoRepeat) {
            root.checked = !root.checked
            root.checkedChanged()
        }
        event.accepted = true
    }
} 