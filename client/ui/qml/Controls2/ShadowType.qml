import QtQuick
import QtQuick.Window
import Qt5Compat.GraphicalEffects

DropShadow {
    id: root

    property real blur: 0

    radius: Math.max(0, root.blur * 1.66665 - 1 / Screen.devicePixelRatio)
    samples: Math.ceil(root.radius) * 2 + 1
    transparentBorder: true
}
