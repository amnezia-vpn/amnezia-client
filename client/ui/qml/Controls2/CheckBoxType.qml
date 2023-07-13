import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import "TextTypes"

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

        anchors.verticalCenter: parent.verticalCenter

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
                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: root.pressed ? pressedImageColor : root.checked ? checkedImageColor : defaultImageColor
                    }
                }
            }
        }
    }

    contentItem: ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 8 + checkBoxBackground.width

        spacing: 4

        ListItemTitleType {
            Layout.fillWidth: true
//            Layout.topMargin: 16
//            Layout.bottomMargin: description.visible ? 0 : 16

            text: root.text
        }

        CaptionTextType {
            id: description

            Layout.fillWidth: true
            Layout.bottomMargin: 16

            text: root.descriptionText
            color: "#878b91"

            visible: root.descriptionText !== ""
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }
}


