import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

RadioButton {
    id: root

    property string headerText
    property string bodyText
    property string footerText

    property string hoveredColor: AmneziaStyle.color.barelyTranslucentWhite
    property string defaultColor: AmneziaStyle.color.transparent
    property string disabledColor: AmneziaStyle.color.transparent
    property string pressedColor: AmneziaStyle.color.barelyTranslucentWhite
    property string selectedColor: AmneziaStyle.color.transparent

    property string textColor: AmneziaStyle.color.midnightBlack

    property string pressedBorderColor: Qt.rgba(251/255, 178/255, 106/255, 0.3)
    property string selectedBorderColor: AmneziaStyle.color.goldenApricot
    property string defaultBodredColor: AmneziaStyle.color.transparent
    property int borderWidth: 0

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    hoverEnabled: true

    indicator: Rectangle {
        anchors.fill: parent
        radius: 16

        color: {
            if (root.enabled) {
                if (root.hovered) {
                    return hoveredColor
                } else if (root.checked) {
                    return selectedColor
                }
                return defaultColor
            } else {
                return disabledColor
            }
        }

        border.color: {
            if (root.enabled) {
                if (root.pressed) {
                    return pressedBorderColor
                } else if (root.checked) {
                    return selectedBorderColor
                }
            }
            return defaultBodredColor
        }

        border.width: {
            if (root.enabled) {
                if(root.checked) {
                    return 1
                }
                return root.pressed ? 1 : 0
            } else {
                return 0
            }
        }

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
        Behavior on border.color {
            PropertyAnimation { duration: 200 }
        }
    }

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 16

        Text {
            text: root.headerText
            wrapMode: Text.WordWrap
            color: AmneziaStyle.color.paleGray
            font.pixelSize: 25
            font.weight: 700
            font.family: "PT Root UI VF"

            height: 30
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.topMargin: 16
        }

        Text {
            text: root.bodyText
            wrapMode: Text.WordWrap
            color: AmneziaStyle.color.paleGray
            font.pixelSize: 16
            font.weight: 400
            font.family: "PT Root UI VF"

            height: 24
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: root.footerText !== "" ? 0 : 16
        }

        Text {
            text: root.footerText
            wrapMode: Text.WordWrap
            visible: root.footerText !== ""
            color: AmneziaStyle.color.mutedGray
            font.pixelSize: 13
            font.weight: 400
            font.family: "PT Root UI VF"

            height: 16
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16
        }
    }

    MouseArea {
        anchors.fill: parent

        cursorShape: Qt.PointingHandCursor
        enabled: false
    }
}
