import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Button {
    id: root

    property string image

    property string hoveredColor: AmneziaStyle.color.translucentWhite
    property string defaultColor: AmneziaStyle.color.transparent
    property string pressedColor: AmneziaStyle.color.sheerWhite
    property string disableColor: AmneziaStyle.color.slateGray

    property string imageColor: AmneziaStyle.color.mutedGray
    property string disableImageColor: AmneziaStyle.color.slateGray

    property alias backgroundColor: background.color
    property alias backgroundRadius: background.radius

    property string borderFocusedColor: AmneziaStyle.color.paleGray
    property int borderFocusedWidth: 1

    hoverEnabled: true

    icon.source: image
    icon.color: root.enabled ? imageColor : disableImageColor

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

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()

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
