import QtQuick
import QtQuick.Effects

import Style 1.0

MultiEffect {
    id: root

    property real blur: 0
    property color color: AmneziaStyle.color.shadowLayer1
    property real horizontalOffset: 0
    property real verticalOffset: 0

    readonly property var blurParams: {
        if (root.blur <= 3) {
            return [12, 0.75]
        }
        if (root.blur <= 4) {
            return [12, 1.0]
        }
        if (root.blur <= 6) {
            return [20, 1.0]
        }
        if (root.blur <= 8) {
            return [48, 0.5]
        }
        if (root.blur <= 15) {
            return [48, 0.75]
        }
        return [48, 1.0]
    }

    shadowEnabled: true
    shadowColor: root.color
    shadowHorizontalOffset: root.horizontalOffset
    shadowVerticalOffset: root.verticalOffset
    blurMax: root.blurParams[0]
    shadowBlur: root.blurParams[1]
}
