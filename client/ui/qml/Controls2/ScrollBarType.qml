import QtQuick
import QtQuick.Controls

import Style 1.0

ScrollBar {
    id: root

    policy: ScrollBar.AsNeeded

    padding: 2
    minimumSize: 0.1

    contentItem: Rectangle {
        implicitWidth: 4
        implicitHeight: 4

        radius: 2
        color: root.pressed ? AmneziaStyle.color.borderBold : AmneziaStyle.color.borderMedium

        opacity: root.policy === ScrollBar.AlwaysOn || (root.active && root.size < 1.0) ? 1 : 0

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
    }

    background: null
}
