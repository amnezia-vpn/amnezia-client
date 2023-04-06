import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RadioButton {
    id: root

    property string headerText
    property string bodyText
    property string footerText

    property string hoveredColor: Qt.rgba(255, 255, 255, 0)
    property string defaultColor: Qt.rgba(255, 255, 255, 0.05)
    property string disabledColor: Qt.rgba(255, 255, 255, 0)
    property string pressedColor: Qt.rgba(255, 255, 255, 0.05)

    property string textColor: "#0E0E11"

    property string pressedBorderColor: Qt.rgba(251, 178, 106, 0.3)
    property string hoveredBorderColor: Qt.rgba(251, 178, 106, 1)
    property int borderWidth: 0

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    hoverEnabled: true

    indicator: Rectangle {
        anchors.fill: parent
        radius: 16

        color: {
            if (root.enabled) {
                if(root.checked) {
                    return pressedColor
                }
                return hovered ? hoveredColor : defaultColor
            } else {
                return disabledColor
            }
        }
        border.color: {
            if (root.enabled) {
                if(root.checked) {
                    return pressedBorderColor
                }
                return hovered ? hoveredBorderColor : defaultColor
            } else {
                return disabledColor
            }
        }
        border.width: {
            if (root.enabled) {
                if(root.checked) {
                    return 1
                }
                return hovered ? 1 : 0
            } else {
                return 0
            }
        }

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 16

        Text {
            text: root.headerText
            color: "#878b91"
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
            color: "#878b91"
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
            enabled: root.footerText !== ""
            color: "#878b91"
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
