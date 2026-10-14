import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

AbstractButton {
    id: root

    property bool destructive: false

    Layout.fillWidth: true

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: Math.max(48, implicitContentHeight + topPadding + bottomPadding)

    horizontalPadding: 8
    verticalPadding: 12

    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    opacity: root.enabled ? 1 : 0.4

    Accessible.role: Accessible.MenuItem
    Accessible.name: root.text

    background: Rectangle {
        radius: 12
        color: root.enabled && (root.hovered || root.down) ? AmneziaStyle.color.bgStateLayer : AmneziaStyle.color.transparent

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        FocusRingType {
            radius: 12
            visible: root.visualFocus
        }
    }

    contentItem: BodyTextType {
        text: root.text
        color: root.destructive ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.textPrimary
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
