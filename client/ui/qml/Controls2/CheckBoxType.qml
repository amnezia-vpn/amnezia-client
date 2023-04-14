import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Item {
    id: root

    property string hoveredColor: Qt.rgba(1, 1, 1, 0.05)
    property string defaultColor: "transparent"
    property string pressedColor: Qt.rgba(1, 1, 1, 0.05)

    property string defaultBorderColor: "#D7D8DB"
    property string checkedBorderColor: "#FBB26A"

    property string checkedImageColor: "#FBB26A"
    property string hoveredImageColor: "#A85809"
    property string defaultImageColor: "transparent"

    property string imageSource: "qrc:/images/controls/check.svg"

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
                ColorOverlay {
                    id: imageColor
                    anchors.fill: indicator
                    source: indicator
                }
            }

            Rectangle {
                id: imageBorder

                anchors.verticalCenter: checkBox.verticalCenter
                anchors.horizontalCenter: checkBox.horizontalCenter
                width: 24
                height: 24
                color: "transparent"
                border.color: checkBox.checked ? checkedBorderColor : defaultBorderColor
                border.width: 1
                radius: 4
            }

            background: Rectangle {
                id: checkBoxBackground
                radius: 16

                color: "transparent"

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
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
        hoverEnabled: true

        onEntered: {
            checkBoxBackground.color = hoveredColor
        }

        onExited: {
            checkBoxBackground.color = defaultColor
        }

        onPressedChanged: {
            indicator.source = pressed ? imageSource : ""
            imageColor.color = pressed ? hoveredImageColor : defaultImageColor
            checkBoxBackground.color = pressed ? pressedColor : entered ? hoveredColor : defaultColor
        }

        onClicked: {
            checkBox.checked = !checkBox.checked
            indicator.source = checkBox.checked ? imageSource : ""
            imageColor.color = checkBox.checked ? checkedImageColor : defaultImageColor
            imageBorder.border.color = checkBox.checked ? checkedBorderColor : defaultBorderColor
        }
    }
}
