import QtQuick
import QtQuick.Templates as T

import Style 1.0

Item {
    id: root

    property T.AbstractButton control: null
    property bool showSeparator: true
    property color separatorColor: AmneziaStyle.color.borderSoft

    readonly property bool active: root.control !== null && root.control.enabled

    implicitHeight: 56

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
        enabled: root.active
    }

    Item {
        anchors.fill: parent
        anchors.topMargin: 4
        anchors.bottomMargin: 4

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
