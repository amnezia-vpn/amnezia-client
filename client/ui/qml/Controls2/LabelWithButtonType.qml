import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

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
    property bool isLeftImageHoverEnabled: true
    property bool isSmallLeftImage: false

    property alias rightButton: rightImage
    property alias eyeButton: eyeImage
    property FlickableType parentFlickable

    property string textColor: AmneziaStyle.color.paleGray
    property string textDisabledColor: AmneziaStyle.color.mutedGray
    property string descriptionColor: AmneziaStyle.color.mutedGray
    property string descriptionDisabledColor: AmneziaStyle.color.charcoalGray
    property real textOpacity: 1.0

    property string borderFocusedColor: AmneziaStyle.color.paleGray
    property int borderFocusedWidth: 1

    property string rightImageColor: AmneziaStyle.color.paleGray

    property bool descriptionOnTop: false
    property bool hideDescription: true

    property bool isFocusable: !(eyeImage.visible || rightImage.visible) // TODO: this component already has focusable items

    Keys.onTabPressed: {
        FocusController.nextKeyTabItem()
    }

    Keys.onBacktabPressed: {
        FocusController.previousKeyTabItem()
    }

    Keys.onUpPressed: {
        FocusController.nextKeyUpItem()
    }
    
    Keys.onDownPressed: {
        FocusController.nextKeyDownItem()
    }
    
    Keys.onLeftPressed: {
        FocusController.nextKeyLeftItem()
    }

    Keys.onRightPressed: {
        FocusController.nextKeyRightItem()
    }
    
    implicitWidth: content.implicitWidth + content.anchors.topMargin + content.anchors.bottomMargin
    implicitHeight: content.implicitHeight + content.anchors.leftMargin + content.anchors.rightMargin

    onFocusChanged: {
        if (root.activeFocus) {
            if (root.parentFlickable) {
                root.parentFlickable.ensureVisible(root)
            }
        }
    }

    Connections {
        target: rightImage
        function onFocusChanged() {
            if (rightImage.activeFocus) {
                if (root.parentFlickable) {
                    root.parentFlickable.ensureVisible(root)
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: root.enabled

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

            Layout.preferredHeight: (rightImageSource || !isLeftImageHoverEnabled || isSmallLeftImage) ? 40 : 56
            Layout.preferredWidth: (rightImageSource || !isLeftImageHoverEnabled || isSmallLeftImage)? 40 : 56
            Layout.rightMargin: isSmallLeftImage ? 8 : (rightImageSource || !isLeftImageHoverEnabled) ? 16 : 0

            radius: 12
            color: AmneziaStyle.color.transparent

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

            hoverEnabled: true
            image: buttonImageSource
            imageColor: rightImageColor

            Layout.alignment: Qt.AlignRight

            Rectangle {
                id: eyeImageBackground
                anchors.fill: parent
                radius: 12
                color: AmneziaStyle.color.transparent

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }

            onClicked: {
                hideDescription = !hideDescription
            }

            Keys.onEnterPressed: {
                clicked()
            }

            Keys.onReturnPressed: {
                clicked()
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
                color: AmneziaStyle.color.transparent

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }
            onClicked: {
                if (clickedFunction && typeof clickedFunction === "function") {
                    clickedFunction()
                }
            }
        }
    }

    Rectangle {
        id: background
        anchors.fill: root
        color: AmneziaStyle.color.transparent

        border.color: root.activeFocus ? root.borderFocusedColor : AmneziaStyle.color.transparent
        border.width: root.activeFocus ? root.borderFocusedWidth : 0


        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    Keys.onEnterPressed: {
        if (clickedFunction && typeof clickedFunction === "function") {
            clickedFunction()
        }
    }

    Keys.onReturnPressed: {
        if (clickedFunction && typeof clickedFunction === "function") {
            clickedFunction()
        }
    }
}
