import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import Style 1.0

Button {
    id: root

    enum Size { Small, Regular }

    property int size: IconButtonType.Regular
    property bool filled: false

    implicitWidth: d.box
    implicitHeight: d.box

    padding: 0

    hoverEnabled: true
    display: AbstractButton.IconOnly

    icon.width: d.iconSize
    icon.height: d.iconSize
    icon.color: root.filled ? AmneziaStyle.color.iconInverted : AmneziaStyle.color.iconDefault

    opacity: root.enabled ? 1 : 0.4

    QtObject {
        id: d

        readonly property int box: root.size === IconButtonType.Small ? 36 : 48
        readonly property int iconSize: root.size === IconButtonType.Small ? 20 : 24
    }

    background: Item {
        implicitWidth: d.box
        implicitHeight: d.box

        Rectangle {
            anchors.fill: parent
            anchors.margins: root.filled && root.down ? 2 : 0

            radius: root.filled && root.down ? 10 : 12
            color: {
                if (root.filled) {
                    return root.hovered || root.down ? AmneziaStyle.color.bgSurfaceInverseAlt : AmneziaStyle.color.bgSurfaceInverse
                }
                if (root.down) {
                    return AmneziaStyle.color.bgStateLayerPressed
                }
                return root.hovered ? AmneziaStyle.color.bgStateLayer : AmneziaStyle.color.transparent
            }
            border.width: !root.filled && root.down ? 1 : 0
            border.color: AmneziaStyle.color.borderMedium

            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }

        Rectangle {
            anchors.fill: parent

            visible: root.filled && root.down
            radius: 12
            color: AmneziaStyle.color.transparent
            border.width: 1
            border.color: AmneziaStyle.color.borderEmphasis
        }

        FocusRingType {
            radius: 12
            visible: root.visualFocus
        }
    }

    contentItem: Item {
        implicitWidth: root.icon.width
        implicitHeight: root.icon.height

        IconImage {
            anchors.centerIn: parent

            width: root.icon.width
            height: root.icon.height

            source: root.icon.source
            sourceSize: Qt.size(root.icon.width, root.icon.height)
            color: root.icon.color
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
