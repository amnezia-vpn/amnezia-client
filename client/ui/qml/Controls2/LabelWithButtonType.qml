import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"
import "../Components"

Item {
    id: root

    property string text
    property int textMaximumLineCount: 2
    property int textElide: Qt.ElideRight

    property string descriptionText

    property var clickedFunction

    property string buttonImageSource
    property string rightImageSource
    property string leftImageSource
    property bool isLeftImageHoverEnabled: true //todo separete this qml file to 3

    property string textColor: "#d7d8db"
    property string textDisabledColor: "#878B91"
    property string descriptionColor: "#878B91"
    property string descriptionDisabledColor: "#494B50"
    property real textOpacity: 1.0

    property string rightImageColor: "#d7d8db"

    property bool descriptionOnTop: false
    property bool hideDescription: true

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

            Layout.preferredHeight: rightImageSource || !isLeftImageHoverEnabled ? leftImage.implicitHeight : 56
            Layout.preferredWidth: rightImageSource || !isLeftImageHoverEnabled ? leftImage.implicitWidth : 56
            Layout.rightMargin: rightImageSource || !isLeftImageHoverEnabled ? 16 : 0

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
                color: {
                    if (root.enabled) {
                        return root.descriptionOnTop ? root.descriptionColor : root.textColor
                    } else {
                        return root.descriptionOnTop ? root.descriptionDisabledColor : root.textDisabledColor
                    }
                }

                maximumLineCount: root.textMaximumLineCount
                elide: root.textElide

                opacity: root.textOpacity

                Layout.fillWidth: true

                lineHeight: root.descriptionOnTop ? parent.descriptionTextLineHeight : parent.textLineHeight
                font.pixelSize: root.descriptionOnTop ? parent.descriptionTextSize : parent.textPixelSize
                font.letterSpacing: root.descriptionOnTop ? 0.02 : 0

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                Behavior on opacity {
                    PropertyAnimation { duration: 200 }
                }
            }


            CaptionTextType {
                id: description

                text: (eyeImage.visible && hideDescription) ? replaceWithAsterisks(root.descriptionText) : root.descriptionText
                color: {
                    if (root.enabled) {
                        return root.descriptionOnTop ? root.textColor : root.descriptionColor
                    } else {
                        return root.descriptionOnTop ? root.textDisabledColor : root.descriptionDisabledColor
                    }
                }

                opacity: root.textOpacity

                visible: root.descriptionText !== ""

                Layout.fillWidth: true

                lineHeight: root.descriptionOnTop ? parent.textLineHeight : parent.descriptionTextLineHeight
                font.pixelSize: root.descriptionOnTop ? parent.textPixelSize : parent.descriptionTextSize
                font.letterSpacing: root.descriptionOnTop ? 0 : 0.02

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                Behavior on opacity {
                    PropertyAnimation { duration: 200 }
                }

                function replaceWithAsterisks(input) {
                    return '*'.repeat(input.length)
                }
            }
        }

        ImageButtonType {
            id: eyeImage
            visible: buttonImageSource !== ""

            implicitWidth: 40
            implicitHeight: 40

            hoverEnabled: false
            image: buttonImageSource
            imageColor: rightImageColor

            Layout.alignment: Qt.AlignRight

            Rectangle {
                id: eyeImageBackground
                anchors.fill: parent
                radius: 12
                color: "transparent"

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: root.enabled

                onEntered: {
                    if (buttonImageSource) {
                        eyeImageBackground.color = rightImage.hoveredColor
                    }
                    root.textOpacity = 0.8
                }

                onExited: {
                    if (buttonImageSource) {
                        eyeImageBackground.color = rightImage.defaultColor
                    }
                    root.textOpacity = 1
                }

                onPressedChanged: {
                    if (buttonImageSource) {
                        eyeImageBackground.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
                    }
                    root.textOpacity = 0.7
                }

                onClicked: {
                    hideDescription = !hideDescription
                }
            }
        }

        ImageButtonType {
            id: rightImage

            implicitWidth: 40
            implicitHeight: 40

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

            Loader {
                anchors.fill: parent
                sourceComponent: rightImageMouseArea
                property bool mouseAreaEnabled: eyeImage.visible
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

    Loader {
        anchors.fill: parent
        sourceComponent: rightImageMouseArea
        property bool mouseAreaEnabled: !eyeImage.visible
    }

    Component {
        id: rightImageMouseArea

        MouseArea {
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: root.enabled

            enabled: mouseAreaEnabled

            onEntered: {
                if (rightImageSource) {
                    rightImageBackground.color = rightImage.hoveredColor
                } else if (leftImageSource) {
                    leftImageBackground.color = rightImage.hoveredColor
                }
                root.textOpacity = 0.8
            }

            onExited: {
                if (rightImageSource) {
                    rightImageBackground.color = rightImage.defaultColor
                } else if (leftImageSource) {
                    leftImageBackground.color = rightImage.defaultColor
                }
                root.textOpacity = 1
            }

            onPressedChanged: {
                if (rightImageSource) {
                    rightImageBackground.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
                } else if (leftImageSource) {
                    leftImageBackground.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
                }
                root.textOpacity = 0.7
            }

            onClicked: {
                if (clickedFunction && typeof clickedFunction === "function") {
                    clickedFunction()
                }
            }
        }
    }
}
