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

    property string accessibleName: ""

    property alias backgroundColor: background.color
    property alias backgroundRadius: background.radius

    property string borderFocusedColor: AmneziaStyle.color.paleGray
    property int borderFocusedWidth: 1

    hoverEnabled: true

    icon.source: image
    icon.color: root.enabled ? imageColor : disableImageColor

    property bool isFocusable: true

    Accessible.name: accessibleName !== "" ? accessibleName : defaultAccessibleName()
    Accessible.role: Accessible.Button

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

    function defaultAccessibleName() {
        if (image.indexOf("close.svg") !== -1) return qsTr("Close")
        if (image.indexOf("copy.svg") !== -1) return qsTr("Copy")
        if (image.indexOf("qr-code.svg") !== -1) return qsTr("Show QR code")
        if (image.indexOf("trash.svg") !== -1) return qsTr("Delete")
        if (image.indexOf("refresh-cw.svg") !== -1) return qsTr("Refresh")
        if (image.indexOf("more-vertical.svg") !== -1) return qsTr("More options")
        if (image.indexOf("settings") !== -1) return qsTr("Settings")
        if (image.indexOf("plus.svg") !== -1) return qsTr("Add")
        if (image.indexOf("chevron-down.svg") !== -1) return qsTr("Expand")
        if (image.indexOf("chevron-right.svg") !== -1) return qsTr("Open")
        if (image.indexOf("download.svg") !== -1) return qsTr("Download")
        return ""
    }
}
