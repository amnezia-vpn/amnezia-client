import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

ProgressBar {
    id: root

    implicitHeight: 4

    background: Rectangle {
        color: AmneziaStyle.color.richBrown
    }

    contentItem: Item {
        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: AmneziaStyle.color.goldenApricot
        }
    }
}
