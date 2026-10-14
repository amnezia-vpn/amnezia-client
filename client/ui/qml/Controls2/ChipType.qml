import QtQuick
import QtQuick.Controls

import Style 1.0

import "TextTypes"

Button {
    id: root

    property bool applied: false

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: 34

    horizontalPadding: 14
    verticalPadding: 8

    hoverEnabled: true

    opacity: root.enabled ? 1 : 0.5

    background: Rectangle {
        radius: 16

        color: root.applied ? AmneziaStyle.color.accentLight : AmneziaStyle.color.bgSurfaceAlt
        border.width: root.applied || root.checked || (root.hovered && !root.down) ? 1 : 0
        border.color: root.applied || root.checked ? AmneziaStyle.color.accentLight : AmneziaStyle.color.borderBold

        Rectangle {
            anchors.fill: parent

            radius: parent.radius
            visible: root.down && !root.applied
            color: AmneziaStyle.color.bgStateLayer
        }

        FocusRingType {
            radius: 16
            visible: root.visualFocus
        }
    }

    contentItem: SmallTextType {
        text: root.text
        color: {
            if (root.applied) {
                return AmneziaStyle.color.textInverted
            }
            return root.checked ? AmneziaStyle.color.textPrimary : AmneziaStyle.color.textSecondary
        }
        font.weight: root.applied ? 600 : 400
        font.letterSpacing: root.applied ? -0.4 : 0

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
