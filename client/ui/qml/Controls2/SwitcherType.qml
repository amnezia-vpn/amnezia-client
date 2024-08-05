import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Switch {
    id: root

    property alias descriptionText: description.text
    property string descriptionTextColor: AmneziaStyle.color.grey
    property string descriptionTextDisabledColor: AmneziaStyle.color.greyDisabled

    property string textColor: AmneziaStyle.color.white
    property string textDisabledColor: AmneziaStyle.color.grey

    property string checkedIndicatorColor: AmneziaStyle.color.brown
    property string defaultIndicatorColor: AmneziaStyle.color.transparent
    property string checkedDisabledIndicatorColor: AmneziaStyle.color.brownDark

    property string borderFocusedColor: AmneziaStyle.color.white
    property int borderFocusedWidth: 1

    property string checkedIndicatorBorderColor: AmneziaStyle.color.brown
    property string defaultIndicatorBorderColor: AmneziaStyle.color.greyDisabled
    property string checkedDisabledIndicatorBorderColor: AmneziaStyle.color.brownDark

    property string checkedInnerCircleColor: AmneziaStyle.color.orange
    property string defaultInnerCircleColor: AmneziaStyle.color.white
    property string checkedDisabledInnerCircleColor: AmneziaStyle.color.brownLight
    property string defaultDisabledInnerCircleColor: AmneziaStyle.color.greyDisabled

    property string hoveredIndicatorBackgroundColor: AmneziaStyle.color.blackHovered
    property string defaultIndicatorBackgroundColor: AmneziaStyle.color.transparent

    hoverEnabled: enabled ? true : false
    focusPolicy: Qt.TabFocus

    property FlickableType parentFlickable: null
    onFocusChanged: {
        if (root.activeFocus) {
            if (root.parentFlickable) {
                root.parentFlickable.ensureVisible(root)
            }
        }
    }

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

    contentItem: ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.bottom: parent.bottom
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

    Keys.onEnterPressed: {
        root.checked = !root.checked
        root.checkedChanged()
    }

    Keys.onReturnPressed: {
        root.checked = !root.checked
        root.checkedChanged()
    }
}
