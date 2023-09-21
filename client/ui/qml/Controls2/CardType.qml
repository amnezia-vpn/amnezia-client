import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RadioButton {
    id: root

    property string headerText
    property string bodyText
    property string footerText

    property string hoveredColor: Qt.rgba(1, 1, 1, 0.05)
    property string defaultColor: Qt.rgba(1, 1, 1, 0)
    property string disabledColor: Qt.rgba(1, 1, 1, 0)
    property string pressedColor: Qt.rgba(1, 1, 1, 0.05)
    property string selectedColor: Qt.rgba(1, 1, 1, 0)

    property string textColor: "#0E0E11"

    property string pressedBorderColor: Qt.rgba(251/255, 178/255, 106/255, 0.3)
    property string selectedBorderColor: "#FBB26A"
    property string defaultBodredColor: "transparent"
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
            color: "#D7D8DB"
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
            color: "#D7D8DB"
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
            visible: root.footerText !== ""
            color: "#878B91"
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
}
