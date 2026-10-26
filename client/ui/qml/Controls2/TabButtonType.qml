import QtQuick
import QtQuick.Controls

import Style 1.0

import "TextTypes"

TabButton {
    id: root

    enum Size { Large, Medium }

    property int size: TabButtonType.Large

    readonly property int maximumHorizontalPadding: root.size === TabButtonType.Large ? 24 : 10

    implicitWidth: implicitContentWidth + 2 * root.maximumHorizontalPadding
    implicitHeight: root.size === TabButtonType.Large ? 48 : 36

    horizontalPadding: Math.max(4, Math.min(root.maximumHorizontalPadding, (root.width - implicitContentWidth) / 2))
    verticalPadding: 0

    hoverEnabled: true

    opacity: root.enabled ? 1 : 0.4

    background: Item {
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            height: 1
            color: root.checked ? AmneziaStyle.color.accentLight : AmneziaStyle.color.borderSoft

            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }

        FocusRingType {
            radius: 8
            visible: root.visualFocus
        }
    }

    contentItem: BodyTextType {
        text: root.text
        color: {
            if (root.down) {
                return AmneziaStyle.color.accentDeep
            }
            return root.hovered ? AmneziaStyle.color.textAccent : AmneziaStyle.color.textPrimary
        }

        wrapMode: Text.NoWrap
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: function(event) {
        if (!event.isAutoRepeat) {
            root.checked = true
            root.clicked()
        }
    }

    Keys.onReturnPressed: function(event) {
        if (!event.isAutoRepeat) {
            root.checked = true
            root.clicked()
        }
    }
}
