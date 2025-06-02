import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "TextTypes"

RadioButton {
    id: root

    property int textMaximumLineCount: 2
    property int textElide: Qt.ElideRight
    property string descriptionText

    property string hoveredColor: AmneziaStyle.color.barelyTranslucentWhite
    property string defaultColor: AmneziaStyle.color.transparent
    property string disabledColor: AmneziaStyle.color.transparent
    property string selectedColor: AmneziaStyle.color.transparent

    property string textColor: AmneziaStyle.color.paleGray
    property string textDisabledColor: AmneziaStyle.color.mutedGray
    property string selectedTextColor: AmneziaStyle.color.goldenApricot
    property string selectedTextDisabledColor: AmneziaStyle.color.burntOrange
    property string descriptionColor: AmneziaStyle.color.mutedGray
    property string descriptionDisabledColor: AmneziaStyle.color.charcoalGray

    property string borderFocusedColor: AmneziaStyle.color.paleGray
    property int borderFocusedWidth: 1

    property string imageSource
    property bool showImage

    property bool isFocusable: true

    
    property string radioButtonInnerCirclePressedSource: "qrc:/images/controls/radio-button-inner-circle-pressed.png"
    property string radioButtonInnerCircleSource: "qrc:/images/controls/radio-button-inner-circle.png"
    property string radioButtonPressedSource: "qrc:/images/controls/radio-button-pressed.svg"
    property string radioButtonDefaultSource: "qrc:/images/controls/radio-button.svg"

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

    hoverEnabled: true

    indicator: Rectangle {
        id: background

        anchors.verticalCenter: parent.verticalCenter

        border.color: root.focus ? root.borderFocusedColor : AmneziaStyle.color.transparent
        border.width: root.focus ? root.borderFocusedWidth : 0

        implicitWidth: 56
        implicitHeight: 56
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

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }

        Behavior on border.color {
            PropertyAnimation { duration: 200 }
        }

        Image {
            source: {
                if (showImage) {
                    return imageSource
                } else if (root.pressed) {
                    return root.radioButtonInnerCirclePressedSource
                } else if (root.checked) {
                    return root.radioButtonInnerCircleSource
                }

                return ""
            }

            opacity: root.enabled ? 1.0 : 0.3
            anchors.centerIn: parent

            width: 24
            height: 24
        }

        Image {
            source: {
                if (showImage) {
                    return ""
                } else if (root.pressed || root.checked) {
                    return root.radioButtonPressedSource
                } else {
                    return root.radioButtonDefaultSource
                }
            }

            opacity: root.enabled ? 1.0 : 0.3
            anchors.centerIn: parent

            width: 24
            height: 24
        }
    }

    contentItem: Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 8 + background.width

        implicitHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            spacing: 4

            ListItemTitleType {
                text: root.text
                maximumLineCount: root.textMaximumLineCount
                elide: root.textElide

                color: {
                    if (root.enabled) {
                        return root.checked ? selectedTextColor : textColor
                    } else {
                        return root.checked ? selectedTextDisabledColor : textDisabledColor
                    }
                }

                Layout.fillWidth: true

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }

            CaptionTextType {
                id: description

                color: root.enabled ? root.descriptionColor : root.descriptionDisabledColor
                text: root.descriptionText

                visible: root.descriptionText !== ""

                Layout.fillWidth: true
            }
        }
    }

    MouseArea {
        anchors.fill: root
        cursorShape: Qt.PointingHandCursor
        preventStealing: false
        enabled: false
    }
}


