import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import Style 1.0

TabButton {
    id: root

    property bool showBadge: false

    implicitWidth: 44
    implicitHeight: 44
    width: 44
    height: 44

    hoverEnabled: true

    padding: 0

    icon.width: 24
    icon.height: 24
    icon.color: root.checked ? AmneziaStyle.color.accentLight : AmneziaStyle.color.iconDefault

    Accessible.name: root.text

    background: Item {
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
                anchors.horizontalCenter: parent.left
                anchors.horizontalCenterOffset: 20.5
                anchors.verticalCenter: parent.top
                anchors.verticalCenterOffset: 3.5

                visible: root.showBadge
                color: AmneziaStyle.color.accentLight
            }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: {
        root.checked = true
        root.clicked()
    }
    Keys.onReturnPressed: {
        root.checked = true
        root.clicked()
    }
}
