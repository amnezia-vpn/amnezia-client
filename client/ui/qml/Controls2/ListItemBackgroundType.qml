import QtQuick
import QtQuick.Templates as T

import Style 1.0

Item {
    id: root

    property T.AbstractButton control: null
    property bool selected: false
    property bool showSeparator: true
    property color separatorColor: AmneziaStyle.color.borderSoft

    readonly property bool active: root.control !== null && root.control.enabled
    readonly property bool pressed: root.active && root.control.down
    readonly property bool hovered: root.active && root.control.hovered

    implicitHeight: 72

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
        enabled: root.active
    }

    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 4
        anchors.bottomMargin: 4

        radius: 16
        color: root.hovered || root.pressed ? AmneziaStyle.color.bgStateLayer : AmneziaStyle.color.transparent
        border.width: root.pressed || root.selected ? 1 : 0
        border.color: AmneziaStyle.color.borderBold

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        FocusRingType {
            radius: 16
            visible: root.control !== null && root.control.visualFocus
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: 1
        visible: root.showSeparator
        color: root.separatorColor
    }
}
