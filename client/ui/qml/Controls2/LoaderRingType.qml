import QtQuick
import QtQuick.Shapes

import Style 1.0

Item {
    id: root

    property int size: 24
    property real strokeWidth: 2
    property color trackColor: AmneziaStyle.color.textTertiary
    property color arcColor: AmneziaStyle.color.textPrimary
    property bool running: visible

    readonly property real ringRadius: size * 0.35

    implicitWidth: size
    implicitHeight: size

    Shape {
        anchors.fill: parent

        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: AmneziaStyle.color.transparent
            strokeColor: root.trackColor
            strokeWidth: root.strokeWidth

            PathAngleArc {
                centerX: root.size / 2
                centerY: root.size / 2
                radiusX: root.ringRadius
                radiusY: root.ringRadius
                startAngle: 0
                sweepAngle: 360
            }
        }
    }

    Shape {
        id: arc

        anchors.fill: parent

        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: AmneziaStyle.color.transparent
            strokeColor: root.arcColor
            strokeWidth: root.strokeWidth
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: root.size / 2
                centerY: root.size / 2
                radiusX: root.ringRadius
                radiusY: root.ringRadius
                startAngle: 0
                sweepAngle: 90
            }
        }

        RotationAnimator on rotation {
            from: 0
            to: 360
            duration: 900
            loops: Animation.Infinite
            running: root.running
        }
    }
}
