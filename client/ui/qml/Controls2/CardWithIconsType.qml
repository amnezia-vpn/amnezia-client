import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Button {
    id: root

    property string headerText
    property string bodyText
    property string footerText

    property string hoveredColor: "#2C2D30"
    property string defaultColor: "#1C1D21"

    property string textColor: "#0E0E11"

    property string rightImageSource
    property string rightImageColor: "#d7d8db"

    property real textOpacity: 1.0

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

    contentItem: Item {
        anchors.left: parent.left
        anchors.right: parent.right

        implicitHeight: content.implicitHeight
        RowLayout {
            id: content
            anchors.fill: parent

            ColumnLayout {
                ListItemTitleType {
                    text: root.headerText
                    visible: text !== ""

                    Layout.fillWidth: true
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16
                    Layout.topMargin: 16

                    opacity: root.textOpacity
                }

                CaptionTextType {
                    text: root.bodyText
                    visible: text !== ""

                    color: "#878B91"

                    Layout.fillWidth: true
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16
                    Layout.bottomMargin: root.footerText !== "" ? 0 : 16

                    opacity: root.textOpacity
                }

                ButtonTextType {
                    text: root.footerText
                    visible: text !== ""

                    color: "#878B91"

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
                    if (clickedFunction && typeof clickedFunction === "function") {
                        clickedFunction()
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent

        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

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
