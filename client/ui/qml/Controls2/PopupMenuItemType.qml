import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

AbstractButton {
    id: root

    property bool destructive: false
    property string iconSource

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

    contentItem: RowLayout {
        spacing: 12

        IconImage {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24

            visible: root.iconSource !== ""
            source: root.iconSource
            sourceSize: Qt.size(24, 24)
            color: root.destructive ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.iconDefault
        }

        BodyTextType {
            Layout.fillWidth: true

            text: root.text
            color: root.destructive ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.textPrimary
            verticalAlignment: Text.AlignVCenter
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
