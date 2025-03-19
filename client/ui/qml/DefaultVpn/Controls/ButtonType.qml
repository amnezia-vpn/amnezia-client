pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Config 1.0

import "TextTypes"

Button {
    id: root

    property string defaultBackgroundColor: Style.color.white
    property string defaultBorderColor: Style.color.gray3
    property string defaultTextColor: Style.color.accent1
    property string defaultImageColor: Style.color.accent1

    property string hoveredBackgroundColor: Style.color.gray1
    property string hoveredBorderColor: Style.color.gray3
    property string hoveredTextColor: Style.color.accent2
    property string hoveredImageColor: Style.color.accent2

    property string pressedBackgroundColor: Style.color.gray2
    property string pressedBorderColor: Style.color.gray3
    property string pressedTextColor: Style.color.accent3
    property string pressedImageColor: Style.color.accent3

    property string disabledBackgroundColor: Style.color.white
    property string disabledBorderColor: Style.color.gray3
    property string disabledTextColor: Style.color.gray8
    property string disabledImageColor: Style.color.gray8

    property int defaultBorderWidth: 0
    property int disabledBorderWidth: 0
    property int hoveredBorderWidth: 0

    property string imageSource: ""

    readonly property bool isImageOnly: root.text !== ""

    background: Rectangle {
        id: background

        anchors.fill: parent

        radius: 6

        color: root.enabled ? root.defaultBackgroundColor : root.disabledBackgroundColor
        border.color: root.enabled ? root.defaultBorderColor : root.disabledBorderColor
        border.width: root.enabled ? root.defaultBorderWidth : root.disabledBorderWidth
    }

    MouseArea {
        id: mouseArea

        anchors.fill: background
        cursorShape: Qt.PointingHandCursor

        hoverEnabled: true
        enabled: root.enabled

        onEntered: {
            background.color = root.hoveredBackgroundColor
            background.border.color = root.hoveredBorderColor
            background.border.width = root.hoveredBorderWidth
            image.imageColor = root.hoveredImageColor
            buttonText.color = root.hoveredTextColor
        }

        onExited: {
            background.color = root.defaultBackgroundColor
            background.border.color = root.defaultBorderColor
            background.border.width = root.defaultBorderWidth
            image.imageColor = root.defaultImageColor
            buttonText.color = root.defaultTextColor
        }

        onPressedChanged: {
            if (pressed) {
                background.color = root.pressedBackgroundColor
                background.border.color = root.pressedBorderColor
                image.imageColor = root.pressedImageColor
                buttonText.color = root.pressedTextColor
            } else if (entered) {
                background.color = root.hoveredBackgroundColor
                background.border.color = root.hoveredBorderColor
                image.imageColor = root.hoveredImageColor
                buttonText.color = root.hoveredTextColor
            } else {
                background.color = root.defaultBackgroundColor
                background.border.color = root.defaultBorderColor
                image.imageColor = root.defaultImageColor
                buttonText.color = root.defaultTextColor
            }
        }

        onClicked: {
            root.clicked()
        }
    }

    contentItem: Item {
        implicitWidth: content.implicitWidth
        implicitHeight: content.implicitHeight

        RowLayout {
            id: content
            anchors.fill: parent

            MediumTextType {
                id: buttonText

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                visible: root.isImageOnly

                color: root.defaultTextColor
                text: root.text

                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }

            Image {
                id: image

                property color imageColor: root.enabled ? root.defaultImageColor : root.disabledImageColor

                Layout.preferredHeight: 22
                Layout.preferredWidth: 22
                Layout.alignment: Qt.AlignCenter
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                Layout.leftMargin: 12
                Layout.rightMargin: 12

                source: root.imageSource
                visible: root.imageSource === "" ? false : true

                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: image.imageColor
                    }
                }
            }
        }
    }
}
