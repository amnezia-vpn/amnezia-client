import QtQuick
import QtQuick.Shapes

import Style 1.0

Shape {
    id: root

    property color color
    property real radius
    property real startAngle: 0
    property real sweepAngle: 360
    property real strokeWidth: 2
    property var shadowSigmas: [2, 4, 7.3]
    property real step: 0.5

    readonly property var layers: {
        const halfWidth = root.strokeWidth / 2
        const maxSigma = Math.max.apply(null, root.shadowSigmas)
        const reach = halfWidth + 3 * maxSigma
        const count = Math.ceil((reach - halfWidth) / root.step)

        const erf = function(x) {
            const sign = x < 0 ? -1 : 1
            const t = 1 / (1 + 0.3275911 * Math.abs(x))
            const y = 1 - ((((1.061405429 * t - 1.453152027) * t + 1.421413741) * t - 0.284496736) * t + 0.254829592) * t * Math.exp(-x * x)
            return sign * y
        }

        const coverage = function(x) {
            let transparency = 1
            for (let i = 0; i < root.shadowSigmas.length; ++i) {
                const s = root.shadowSigmas[i] * Math.SQRT2
                const a = 0.5 * (erf((x + halfWidth) / s) - erf((x - halfWidth) / s))
                transparency *= 1 - a
            }
            return 1 - transparency
        }

        const result = []
        let outer = 0
        for (let k = count; k >= 1; --k) {
            const edge = halfWidth + k * root.step
            const target = coverage(edge - root.step / 2)
            const alpha = 1 - (1 - target) / (1 - outer)
            outer = target
            result.unshift({ width: 2 * edge, alpha: Math.max(0, alpha) })
        }
        return result
    }

    preferredRendererType: Shape.CurveRenderer

    Component {
        id: layerComponent

        ShapePath {
            required property real layerWidth
            required property real layerAlpha

            fillColor: AmneziaStyle.color.transparent
            strokeColor: Qt.alpha(root.color, layerAlpha)
            strokeWidth: layerWidth
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: root.radius
                radiusY: root.radius
                startAngle: root.startAngle
                sweepAngle: root.sweepAngle
            }
        }
    }

    Component.onCompleted: {
        for (let k = 0; k < root.layers.length; ++k) {
            root.data.push(layerComponent.createObject(root, {
                layerWidth: root.layers[k].width,
                layerAlpha: root.layers[k].alpha
            }))
        }
    }
}
