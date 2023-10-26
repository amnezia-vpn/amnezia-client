import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Popup {
    id: root
    anchors.centerIn: parent

    modal: true
    closePolicy: Popup.NoAutoClose

    visible: false

    Overlay.modal: Rectangle {
        color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
    }

    background: Rectangle {
        color: "transparent"
    }

    BusyIndicator {
        id: busyIndicator

        visible: true
        running: true

        contentItem: Item {
            implicitWidth: 46
            implicitHeight: 46
            transformOrigin: Item.Center

            Shape {
                id: shape
                width: parent.implicitWidth
                height: parent.implicitHeight
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                layer.enabled: true
                layer.samples: 4

                ShapePath {
                    fillColor: "transparent"
                    strokeColor: "#787878"
                    strokeWidth: 3
                    capStyle: ShapePath.RoundCap

                    PathAngleArc {
                        centerX: shape.width / 2
                        centerY: shape.height / 2
                        radiusX: 18
                        radiusY: 18
                        startAngle: 225
                        sweepAngle: -90
                    }
                }
                RotationAnimator {
                    target: shape
                    running: busyIndicator.visible && busyIndicator.running
                    from: 0
                    to: 360
                    loops: Animation.Infinite
                    duration: 1250
                }
            }
        }
    }
}
