import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Button {
    id: root

    property string image

    property string hoveredColor: AmneziaStyle.color.blackHovered
    property string defaultColor: AmneziaStyle.color.transparent
    property string pressedColor: AmneziaStyle.color.blackPressed
    property string disableColor: AmneziaStyle.color.greyDark

    property string imageColor: AmneziaStyle.color.grey
    property string disableImageColor: AmneziaStyle.color.greyDark

    property alias backgroundColor: background.color
    property alias backgroundRadius: background.radius

    property string borderFocusedColor: AmneziaStyle.color.white
    property int borderFocusedWidth: 1

    hoverEnabled: true
    focus: true
    focusPolicy: Qt.TabFocus

    icon.source: image
    icon.color: root.enabled ? imageColor : disableImageColor

    property Flickable parentFlickable

    onFocusChanged: {
        if (root.activeFocus) {
            if (root.parentFlickable) {
                root.parentFlickable.ensureVisible(this)
            }
        }
    }

    Behavior on icon.color {
        PropertyAnimation { duration: 200 }
    }

    background: Rectangle {
        id: background

        anchors.fill: parent
        border.color: root.activeFocus ? root.borderFocusedColor : AmneziaStyle.color.transparent
        border.width: root.activeFocus ? root.borderFocusedWidth : 0

        color: {
            if (root.enabled) {
                if (root.pressed) {
                    return pressedColor
                }
                return hovered ? hoveredColor : defaultColor
            }
            return defaultColor
        }
        radius: 12
        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
        Behavior on border.color {
            PropertyAnimation { duration: 200 }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }
}
