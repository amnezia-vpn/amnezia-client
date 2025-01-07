import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "TextTypes"

Button {
    id: root

    property string hoveredColor: AmneziaStyle.color.lightGray
    property string defaultColor: AmneziaStyle.color.paleGray
    property string disabledColor: AmneziaStyle.color.charcoalGray
    property string pressedColor: AmneziaStyle.color.mutedGray

    property string textColor: AmneziaStyle.color.midnightBlack

    property string borderColor: AmneziaStyle.color.paleGray
    property string borderFocusedColor: AmneziaStyle.color.paleGray
    property int borderWidth: 0
    property int borderFocusedWidth: 1

    property string leftImageSource
    property string rightImageSource
    property string leftImageColor: textColor
    property bool changeLeftImageSize: true

    property bool squareLeftSide: false

    property FlickableType parentFlickable

    property var clickedFunc

    property alias buttonTextLabel: buttonText

    property bool isFocusable: true

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
    
    implicitHeight: 56

    hoverEnabled: true

    onFocusChanged: {
        if (root.activeFocus) {
            if (root.parentFlickable) {
                root.parentFlickable.ensureVisible(this)
            }
        }
    }

    background: Rectangle {
        id: focusBorder

        color: AmneziaStyle.color.transparent
        border.color: root.activeFocus ? root.borderFocusedColor : AmneziaStyle.color.transparent
        border.width: root.activeFocus ? root.borderFocusedWidth : 0

        anchors.fill: parent

        radius: 16

        Rectangle {
            id: background

            anchors.fill: focusBorder
            anchors.margins: root.activeFocus ? 2 : 0

            radius: root.activeFocus ? 14 : 16
            color: {
                if (root.enabled) {
                    if (root.pressed) {
                        return pressedColor
                    }
                    return root.hovered ? hoveredColor : defaultColor
                } else {
                    return disabledColor
                }
            }
            border.color: borderColor
            border.width: borderWidth

            Behavior on color {
                PropertyAnimation { duration: 200 }
            }

            Rectangle {
                visible: root.squareLeftSide

                z: 1

                width: parent.radius
                height: parent.radius
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                color: {
                    if (root.enabled) {
                        if (root.pressed) {
                            return pressedColor
                        }
                        return root.hovered ? hoveredColor : defaultColor
                    } else {
                        return disabledColor
                    }
                }

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: focusBorder
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Item {
        anchors.fill: focusBorder

        implicitWidth: content.implicitWidth
        implicitHeight: content.implicitHeight

        RowLayout {
            id: content
            anchors.centerIn: parent

            Image {
                id: leftImage
                source: root.leftImageSource
                visible: root.leftImageSource === "" ? false : true

                layer {
                    enabled: leftImageColor !== "" ? true : false
                    effect: ColorOverlay {
                        color: leftImageColor
                    }
                }

                Component.onCompleted: {
                    if (root.changeLeftImageSize) {
                        leftImage.Layout.preferredHeight = 20
                        leftImage.Layout.preferredWidth = 20
                    }
                }
            }

            ButtonTextType {
                id: buttonText

                color: root.textColor
                text: root.text
                visible: root.text === "" ? false : true

                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            Image {
                Layout.preferredHeight: 20
                Layout.preferredWidth: 20

                source: root.rightImageSource
                visible: root.rightImageSource === "" ? false : true

                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: textColor
                    }
                }
            }
        }
    }

    Keys.onEnterPressed: {
        if (root.clickedFunc && typeof root.clickedFunc === "function") {
            root.clickedFunc()
        }
    }

    Keys.onReturnPressed: {
        if (root.clickedFunc && typeof root.clickedFunc === "function") {
            root.clickedFunc()
        }
    }

    onClicked: {
        if (root.clickedFunc && typeof root.clickedFunc === "function") {
            root.clickedFunc()
        }
    }
}
