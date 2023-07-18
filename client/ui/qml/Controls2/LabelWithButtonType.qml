import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Item {
    id: root

    property string text
    property string descriptionText

    property var clickedFunction

    property string rightImageSource
    property string leftImageSource

    property string textColor: "#d7d8db"
    property string descriptionColor: "#878B91"
    property string rightImageColor: "#878B91"

    property bool descriptionOnTop: false

    implicitWidth: content.implicitWidth + content.anchors.topMargin + content.anchors.bottomMargin
    implicitHeight: content.implicitHeight + content.anchors.leftMargin + content.anchors.rightMargin

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 16
        anchors.bottomMargin: 16

        Rectangle {
            id: leftImageBackground

            visible: leftImageSource ? true : false

            Layout.preferredHeight: rightImageSource ? leftImage.implicitHeight : 56
            Layout.preferredWidth: rightImageSource ? leftImage.implicitWidth : 56
            Layout.rightMargin: rightImageSource ? 16 : 0

            radius: 12
            color: "transparent"

            Behavior on color {
                PropertyAnimation { duration: 200 }
            }

            Image {
                id: leftImage

                anchors.centerIn: parent
                source: leftImageSource
            }
        }

        ColumnLayout {
            property real textLineHeight: 21.6
            property real descriptionTextLineHeight: 16

            property int textPixelSize: 18
            property int descriptionTextSize: 13

            ListItemTitleType {
                text: root.text
                color: root.descriptionOnTop ? root.descriptionColor : root.textColor

                Layout.fillWidth: true

                lineHeight: root.descriptionOnTop ? parent.descriptionTextLineHeight : parent.textLineHeight
                font.pixelSize: root.descriptionOnTop ? parent.descriptionTextSize : parent.textPixelSize
                font.letterSpacing: root.descriptionOnTop ? 0.02 : 0

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }


            CaptionTextType {
                id: description

                color: root.descriptionOnTop ? root.textColor : root.descriptionColor
                text: root.descriptionText

                visible: root.descriptionText !== ""

                Layout.fillWidth: true

                lineHeight: root.descriptionOnTop ? parent.textLineHeight : parent.descriptionTextLineHeight
                font.pixelSize: root.descriptionOnTop ? parent.textPixelSize : parent.descriptionTextSize
                font.letterSpacing: root.descriptionOnTop ? 0 : 0.02

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
        }

        ImageButtonType {
            id: rightImage

            hoverEnabled: false
            image: rightImageSource
            imageColor: rightImageColor
            visible: rightImageSource ? true : false

            Layout.alignment: Qt.AlignRight

            Rectangle {
                id: rightImageBackground
                anchors.fill: parent
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
            if (rightImageSource) {
                rightImageBackground.color = rightImage.hoveredColor
            } else if (leftImageSource) {
                leftImageBackground.color = rightImage.hoveredColor
            } else {
                background.color = rightImage.hoveredColor
            }
        }

        onExited: {
            if (rightImageSource) {
                rightImageBackground.color = rightImage.defaultColor
            } else if (leftImageSource) {
                leftImageBackground.color = rightImage.defaultColor
            } else {
                background.color = rightImage.defaultColor
            }
        }

        onPressedChanged: {
            if (rightImageSource) {
                rightImageBackground.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
            } else if (leftImageSource) {
                leftImageBackground.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
            } else {
                background.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
            }
        }

        onClicked: {
            if (clickedFunction && typeof clickedFunction === "function") {
                clickedFunction()
            }
        }
    }
}
