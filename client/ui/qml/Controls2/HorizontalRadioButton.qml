import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RadioButton {
    id: root

    property string hoveredColor: Qt.rgba(1, 1, 1, 0.05)
    property string defaultColor: Qt.rgba(1, 1, 1, 0)
    property string disabledColor: Qt.rgba(1, 1, 1, 0)
    property string selectedColor: Qt.rgba(1, 1, 1, 0)

    property string textColor: "#0E0E11"

    property string pressedBorderColor: "#494B50"
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
            text: root.text
            wrapMode: Text.WordWrap
            color: "#D7D8DB"
            font.pixelSize: 16
            font.weight: 400
            font.family: "PT Root UI VF"

            height: 24
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.topMargin: 16
            Layout.bottomMargin: 16

            horizontalAlignment: Qt.AlignHCenter
        }
    }
}
