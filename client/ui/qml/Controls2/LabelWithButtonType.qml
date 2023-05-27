import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Item {
    id: root

    property string text
    property string descriptionText

    property var clickedFunction

    property alias buttonImage: button.image
    property string iconImage

    property string textColor: "#d7d8db"

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        Image {
            id: icon
            source: iconImage
            visible: iconImage ? true : false
            Layout.rightMargin: visible ? 16 : 0
        }

        ColumnLayout {
            ListItemTitleType {
                text: root.text
                color: textColor

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.bottomMargin: description.visible ? 0 : 16

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            CaptionTextType {
                id: description

                color: "#878B91"
                text: root.descriptionText

                visible: root.descriptionText !== ""

                Layout.fillWidth: true
                Layout.bottomMargin: 16

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
        }

        ImageButtonType {
            id: button

            hoverEnabled: false
            image: buttonImage
            onClicked: {
                if (clickedFunction && typeof clickedFunction === "function") {
                    clickedFunction()
                }
            }

            Layout.alignment: Qt.AlignRight

            Rectangle {
                id: imageBackground
                anchors.fill: button
                radius: 12
                color: "transparent"


                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }
        }
    }

    Rectangle {
        id: background
        anchors.fill: root
        color: "transparent"


        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onEntered: {
            if (buttonImage) {
                imageBackground.color = button.hoveredColor
            } else {
                background.color = button.hoveredColor
            }
        }

        onExited: {
            if (buttonImage) {
                imageBackground.color = button.defaultColor
            } else {
                background.color = button.defaultColor
            }
        }

        onPressedChanged: {
            if (buttonImage) {
                imageBackground.color = pressed ? button.pressedColor : entered ? button.hoveredColor : button.defaultColor
            } else {
                background.color = pressed ? button.pressedColor : entered ? button.hoveredColor : button.defaultColor
            }
        }

        onClicked: {
            button.clicked()
        }
    }
}
