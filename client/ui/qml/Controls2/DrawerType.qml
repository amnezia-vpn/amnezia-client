import QtQuick
import QtQuick.Controls

Drawer {
    edge: Qt.BottomEdge

    clip: true
    modal: true

    enter: Transition {
        SmoothedAnimation {
            velocity: 4
        }
    }
    exit: Transition {
        SmoothedAnimation {
            velocity: 4
        }
    }
}
