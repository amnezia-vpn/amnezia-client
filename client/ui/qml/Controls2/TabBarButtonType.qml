import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import Style 1.0

TabButton {
    id: root

    property bool showBadge: false

    hoverEnabled: true

    topPadding: 4
    bottomPadding: 4
    leftPadding: 0
    rightPadding: 0

    icon.width: 24
    icon.height: 24
    icon.color: root.checked ? AmneziaStyle.color.accentLight : AmneziaStyle.color.iconDefault

    Accessible.name: root.text

    background: Item {
        implicitHeight: 32

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

            DotBadgeType {
                anchors.horizontalCenter: parent.right
                anchors.verticalCenter: parent.top

                visible: root.showBadge
                color: AmneziaStyle.color.accentDefault
            }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
