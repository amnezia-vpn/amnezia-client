import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string headerText
    property string textFieldText
    property string textFieldPlaceholderText
    property bool textFieldEditable: true

    property string buttonText
    property var clickedFunc

    property alias textField: textField

    implicitHeight: 74

    Rectangle {
        id: backgroud
        anchors.fill: parent
        color: "#1c1d21"
        radius: 16
        border.color: textField.focus ? "#d7d8db" : "#2C2D30"
        border.width: 1

        Behavior on border.color {
            PropertyAnimation { duration: 200 }
        }
    }

    RowLayout {
        anchors.fill: backgroud
        ColumnLayout {

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

                placeholderText: textFieldPlaceholderText
                placeholderTextColor: "#494B50"

                selectionColor:  "#412102"
                selectedTextColor: "#D7D8DB"

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

        BasicButtonType {
            visible: root.buttonText !== ""

            defaultColor: "transparent"
            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
            pressedColor: Qt.rgba(1, 1, 1, 0.12)
            disabledColor: "#878B91"
            textColor: "#D7D8DB"
            borderWidth: 0

            text: buttonText

            Layout.rightMargin: 24

            onClicked: {
                if (clickedFunc && typeof clickedFunc === "function") {
                    clickedFunc()
                }
            }
        }
    }
}
