import QtQuick

import Style 1.0

Rectangle {
    id: root

    property bool hovered: false
    property bool pressed: false

    width: 48
    height: 48

    radius: 12
    color: {
        if (root.pressed) {
            return AmneziaStyle.color.bgStateLayerPressed
        }
        return root.hovered ? AmneziaStyle.color.bgStateLayer : AmneziaStyle.color.transparent
    }

    Behavior on color {
        ColorAnimation { duration: 150 }
    }
}
