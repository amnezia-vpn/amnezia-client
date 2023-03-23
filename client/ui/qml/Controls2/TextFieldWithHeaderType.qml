import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string headerText
    property string textFieldText
    property bool textFieldEditable: true

    implicitWidth: 328
    implicitHeight: 74

    Rectangle {
        id: backgroud
        anchors.fill: parent
        color: "#1c1d21"
        radius: 16
        border.color: "#d7d8db"
        border.width: textField.focus ? 1 : 0
    }

    ColumnLayout {
        anchors.fill: backgroud

        Text {
            text: root.headerText
            color: "#878b91"
            font.pixelSize: 13
            font.weight: 400
            font.family: "PT Root UI VF"
            font.letterSpacing: 0.02

            height: 16
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.topMargin: 16
        }

        TextField {
            id: textField

            enabled: root.textFieldEditable
            text: root.textFieldText
            color: "#d7d8db"
            font.pixelSize: 16
            font.weight: 400
            font.family: "PT Root UI VF"

            height: 24
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16
            Layout.bottomMargin: 16
            topPadding: 0
            rightPadding: 0
            leftPadding: 0
            bottomPadding: 0

            background: Rectangle {
                anchors.fill: parent
                color: "#1c1d21"
            }
        }
    }
}
