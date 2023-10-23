import QtQuick
import QtQuick.Controls

import "../Config"

Drawer {
    id: drawer
    property bool needCloseButton: true

    Connections {
        target: PageController

        function onForceCloseDrawer() {
            visible = false
        }
    }

    edge: Qt.BottomEdge

    clip: true
    modal: true
    dragMargin: -10

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

    background: Rectangle {
        anchors.fill: parent
        anchors.bottomMargin: -radius
        radius: 16
        color: "#1C1D21"

        border.color: "#2C2D30"
        border.width: 1

        Rectangle {
            visible: GC.isMobile()

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 10

            width: 20
            height: 2
            color: "#2C2D30"
        }
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
    }

    onAboutToShow: {
        if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
            PageController.updateNavigationBarColor(0xFF1C1D21)
        }
    }

    onOpened: {
        if (needCloseButton) {
            PageController.drawerOpen()
        }
    }

    onClosed: {
        if (needCloseButton) {
            PageController.drawerClose()
        }

        var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
        if (initialPageNavigationBarColor !== 0xFF1C1D21) {
            PageController.updateNavigationBarColor(initialPageNavigationBarColor)
        }
    }
}
