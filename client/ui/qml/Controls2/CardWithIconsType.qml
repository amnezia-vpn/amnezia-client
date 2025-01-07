import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Button {
    id: root

    property string headerText
    property string bodyText
    property string footerText

    property string hoveredColor: AmneziaStyle.color.slateGray
    property string defaultColor: AmneziaStyle.color.onyxBlack

    property string textColor: AmneziaStyle.color.midnightBlack

    property string rightImageSource
    property string rightImageColor: AmneziaStyle.color.paleGray

    property string leftImageSource

    property real textOpacity: 1.0

    property alias focusItem: rightImage

    property FlickableType parentFlickable

    hoverEnabled: true

    background: Rectangle {
        id: backgroundRect

        anchors.fill: parent
        radius: 16

        color: defaultColor

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    function ensureVisible(item) {
        if (item.activeFocus) {
            if (root.parentFlickable) {
                root.parentFlickable.ensureVisible(root)
            }
        }
    }

    onFocusChanged: {
        ensureVisible(root)
    }

    focusItem.onFocusChanged: {
        root.ensureVisible(focusItem)
    }

    contentItem: Item {
        anchors.left: parent.left
        anchors.right: parent.right

        implicitHeight: content.implicitHeight

        RowLayout {
            id: content

            anchors.fill: parent

            Image {
                id: leftImage
                source: leftImageSource

                visible: leftImageSource !== ""

                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.topMargin: 24
                Layout.bottomMargin: 24
                Layout.leftMargin: 24
            }

            ColumnLayout {

                ListItemTitleType {
                    text: root.headerText
                    visible: text !== ""

                    Layout.fillWidth: true
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: root.bodyText !== "" ? 0 : 16

                    opacity: root.textOpacity
                }

                CaptionTextType {
                    text: root.bodyText
                    visible: text !== ""

                    color: AmneziaStyle.color.mutedGray
                    textFormat: Text.RichText

                    Layout.fillWidth: true
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16
                    Layout.bottomMargin: root.footerText !== "" ? 0 : 16

                    opacity: root.textOpacity
                }

                ButtonTextType {
                    text: root.footerText
                    visible: text !== ""

                    color: AmneziaStyle.color.mutedGray

                    Layout.fillWidth: true
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 16

                    opacity: root.textOpacity
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

                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                Layout.topMargin: 16
                Layout.bottomMargin: 16
                Layout.rightMargin: 16

                Rectangle {
                    id: rightImageBackground

                    anchors.fill: parent
                    radius: 12
                    color: "transparent"

                    Behavior on color {
                        PropertyAnimation { duration: 200 }
                    }
                }

                onClicked: {
                    root.clicked()
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent

        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        enabled: root.enabled

        onEntered: {
            backgroundRect.color = root.hoveredColor

            if (rightImageSource) {
                rightImageBackground.color = rightImage.hoveredColor
            }
            root.textOpacity = 0.8
        }

        onExited: {
            backgroundRect.color = root.defaultColor

            if (rightImageSource) {
                rightImageBackground.color = rightImage.defaultColor
            }
            root.textOpacity = 1
        }

        onPressedChanged: {
            if (rightImageSource) {
                rightImageBackground.color = pressed ? rightImage.pressedColor : entered ? rightImage.hoveredColor : rightImage.defaultColor
            }
            root.textOpacity = 0.7
        }

        onClicked: {
            root.clicked()
        }
    }
}
