import QtQuick
import QtQuick.Controls

import Style 1.0

import "TextTypes"

Button {
    id: root

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: 34

    horizontalPadding: 14
    verticalPadding: 8

    hoverEnabled: true

    opacity: root.enabled ? 1 : 0.4

    background: Rectangle {
        radius: 16

        color: root.checked ? AmneziaStyle.color.bgSelected : AmneziaStyle.color.bgSurfaceAlt
        border.width: root.checked ? 1 : 0
        border.color: AmneziaStyle.color.accentLight

        Rectangle {
            anchors.fill: parent

            radius: parent.radius
            visible: root.down || root.hovered
            color: AmneziaStyle.color.bgStateLayer
        }

        FocusRingType {
            radius: 16
            visible: root.visualFocus
        }
    }

    contentItem: SmallTextType {
        text: root.text
        color: root.checked ? AmneziaStyle.color.textPrimary : AmneziaStyle.color.textSecondary

        wrapMode: Text.NoWrap
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
