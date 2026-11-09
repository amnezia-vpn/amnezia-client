import QtQuick
import QtQuick.Effects

Item {
    id: root

    property Item source
    property color color

    Repeater {
        model: root.visible ? [[24, 1.0, 0], [24, 1.0, 0], [16, 1.0, 1]] : []

        MultiEffect {
            required property var modelData

            x: root.source ? root.source.x : 0
            y: root.source ? root.source.y : 0
            width: root.source ? root.source.width : 0
            height: root.source ? root.source.height : 0

            source: root.source
            shadowEnabled: true
            shadowColor: root.color
            blurMax: modelData[0]
            shadowBlur: modelData[1]
            blurMultiplier: modelData[2]
        }
    }
}
