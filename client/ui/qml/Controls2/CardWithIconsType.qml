import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

RadioButton {
    id: root

    property string headerText
    property string bodyText
    property string footerText

    property string hoveredColor: Qt.rgba(1, 1, 1, 0.05)
    property string defaultColor: Qt.rgba(1, 1, 1, 0)
    property string disabledColor: Qt.rgba(1, 1, 1, 0)
    property string pressedColor: Qt.rgba(1, 1, 1, 0.05)
    property string selectedColor: Qt.rgba(1, 1, 1, 0)

    property string textColor: "#0E0E11"

    property string pressedBorderColor: Qt.rgba(251/255, 178/255, 106/255, 0.3)
    property string selectedBorderColor: "#FBB26A"
    property string defaultBodredColor: "transparent"
    property int borderWidth: 0

    property string rightImageSource
    property string rightImageColor: "#d7d8db"

    property real textOpacity: 1.0

    hoverEnabled: true

    indicator: Rectangle {
        anchors.fill: parent
        radius: 16

        color: {
            if (root.enabled) {
                if (root.hovered) {
                    return hoveredColor
                } else if (root.checked) {
                    return selectedColor
                }
                return defaultColor
            } else {
                return disabledColor
            }
        }

        border.color: {
            if (root.enabled) {
                if (root.pressed) {
                    return pressedBorderColor
                } else if (root.checked) {
                    return selectedBorderColor
                }
            }
            return defaultBodredColor
        }

        border.width: {
            if (root.enabled) {
                if(root.checked) {
                    return 1
                }
                return root.pressed ? 1 : 0
            } else {
                return 0
            }
        }

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
        Behavior on border.color {
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

        MouseArea {
            anchors.fill: parent

            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true

            onEntered: {
                if (rightImageSource) {
                    rightImageBackground.color = rightImage.hoveredColor
                }
                root.textOpacity = 0.8
            }

            onExited: {
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
}
