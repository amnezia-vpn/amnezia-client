import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    RowLayout {
        id: content

        anchors.fill: parent

        CheckBox {
            id: checkBox

            implicitWidth: 56
            implicitHeight: 56

            indicator: Image {
                id: indicator
                anchors.verticalCenter: checkBox.verticalCenter
                anchors.horizontalCenter: checkBox.horizontalCenter
                source: checkBox.checked ? "qrc:/images/controls/check.svg" : ""
            }
            Rectangle {
                anchors.verticalCenter: checkBox.verticalCenter
                anchors.horizontalCenter: checkBox.horizontalCenter
                width: 24
                height: 24
                color: "transparent"
                border.color: "#FBB26A"
                border.width: 1
                radius: 4
            }
            background: Rectangle {
                id: background
                color: Qt.rgba(255, 255, 255, 0.05)
                radius: 16
            }
        }

        ColumnLayout {
            Text {
                text: "Paragraph"
                color: "#D7D8DB"
                font.pixelSize: 18
                font.weight: 400
                font.family: "PT Root UI VF"

                height: 22
                Layout.fillWidth: true
            }

            Text {
                text: "Caption"
                color: "#878b91"
                font.pixelSize: 13
                font.weight: 400
                font.family: "PT Root UI VF"
                font.letterSpacing: 0.02

                height: 16
                Layout.fillWidth: true
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            checkBox.checked = !checkBox.checked
        }
    }
}
