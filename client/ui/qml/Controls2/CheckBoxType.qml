import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "TextTypes"

CheckBox {
    id: root

    property string descriptionText
    property string descriptionTextColor: AmneziaStyle.color.mutedGray
    property string descriptionTextDisabledColor: AmneziaStyle.color.charcoalGray

    property string textColor: AmneziaStyle.color.paleGray
    property string textDisabledColor: AmneziaStyle.color.mutedGray

    property string hoveredColor: AmneziaStyle.color.barelyTranslucentWhite
    property string defaultColor: AmneziaStyle.color.transparent
    property string pressedColor: AmneziaStyle.color.barelyTranslucentWhite

    property string defaultBorderColor: AmneziaStyle.color.paleGray
    property string checkedBorderColor: AmneziaStyle.color.goldenApricot
    property string checkedBorderDisabledColor: AmneziaStyle.color.deepBrown

    property string borderFocusedColor: AmneziaStyle.color.paleGray

    property string checkedImageColor: AmneziaStyle.color.goldenApricot
    property string pressedImageColor: AmneziaStyle.color.burntOrange
    property string defaultImageColor: AmneziaStyle.color.transparent
    property string checkedDisabledImageColor: AmneziaStyle.color.mutedBrown

    property string imageSource: "qrc:/images/controls/check.svg"

    property var parentFlickable
    onFocusChanged: {
        if (root.activeFocus) {
            if (root.parentFlickable) {
                root.parentFlickable.ensureVisible(root)
            }
        }
    }

    hoverEnabled: enabled ? true : false
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        color: AmneziaStyle.color.transparent
        border.color: root.focus ? borderFocusedColor : AmneziaStyle.color.transparent
        border.width: 1
        radius: 16
    }

    indicator: Rectangle {
        id: background

        anchors.verticalCenter: parent.verticalCenter

        implicitWidth: 56
        implicitHeight: 56
        radius: 16

        color:  {
            if (root.hovered) {
                return hoveredColor
            }
            return defaultColor
        }

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }

        Rectangle {
            id: imageBorder

            anchors.centerIn: parent
            width: 24
            height: 24
            color: AmneziaStyle.color.transparent
            border.color: root.checked ?
                              (root.enabled ?
                                   checkedBorderColor :
                                   checkedBorderDisabledColor) :
                              defaultBorderColor
            border.width: 1
            radius: 4

            Image {
                anchors.centerIn: parent

                source: root.pressed ? imageSource : root.checked ? imageSource : ""
                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: {
                            if (root.pressed) {
                                return root.pressedImageColor
                            } else if (root.checked) {
                                if (root.enabled) {
                                    return root.checkedImageColor
                                } else {
                                    return root.checkedDisabledImageColor
                                }
                            } else {
                                return root.defaultImageColor
                            }
                        }
                    }
                }
            }
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
                Layout.fillWidth: true

                text: root.text
                color: root.enabled ? root.textColor : root.textDisabledColor
            }

            CaptionTextType {
                id: description

                Layout.fillWidth: true

                text: root.descriptionText
                color: root.enabled ? root.descriptionTextColor : root.descriptionTextDisabledColor

                visible: root.descriptionText !== ""
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }


    Keys.onEnterPressed: {
        root.checked = !root.checked
    }

    Keys.onReturnPressed: {
        root.checked = !root.checked
    }

}


