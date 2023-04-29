import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

CheckBox {
    id: root

    property string descriptionText

    property string hoveredColor: Qt.rgba(1, 1, 1, 0.05)
    property string defaultColor: "transparent"
    property string pressedColor: Qt.rgba(1, 1, 1, 0.05)

    property string defaultBorderColor: "#D7D8DB"
    property string checkedBorderColor: "#FBB26A"

    property string checkedImageColor: "#FBB26A"
    property string pressedImageColor: "#A85809"
    property string defaultImageColor: "transparent"

    property string imageSource: "qrc:/images/controls/check.svg"

    hoverEnabled: true

    indicator: Rectangle {
        id: checkBoxBackground

        implicitWidth: 56
        implicitHeight: 56
        radius: 16

        color:  {
            if (root.hovered) {
                return hoveredColor
            }
            return defaultColor
        }

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }

        Rectangle {
            id: imageBorder

            anchors.centerIn: parent
            width: 24
            height: 24
            color: "transparent"
            border.color: root.checked ? checkedBorderColor : defaultBorderColor
            border.width: 1
            radius: 4

            Image {
                id: indicator
                anchors.centerIn: parent

                source: root.pressed ? imageSource : root.checked ? imageSource : ""

                ColorOverlay {
                    id: imageColor
                    anchors.fill: indicator
                    source: indicator

                    color: root.pressed ? pressedImageColor : root.checked ? checkedImageColor : defaultImageColor
                }
            }
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 8 + checkBoxBackground.width

        Text {
            text: root.text
            color: "#D7D8DB"
            font.pixelSize: 18
            font.weight: 400
            font.family: "PT Root UI VF"

            height: 22
            Layout.fillWidth: true
        }

        Text {
            text: root.descriptionText
            color: "#878b91"
            font.pixelSize: 13
            font.weight: 400
            font.family: "PT Root UI VF"
            font.letterSpacing: 0.02

            height: 16
            Layout.fillWidth: true
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }
}


