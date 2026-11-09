import QtQuick
import QtQuick.Controls

import Style 1.0

ProgressBar {
    id: root

    implicitHeight: 4

    background: Rectangle {
        implicitHeight: 4

        radius: 2
        color: AmneziaStyle.color.bgPanel
    }

    contentItem: Item {
        implicitHeight: 4

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height

            radius: 2
            color: AmneziaStyle.color.accentLight
        }
    }
}
