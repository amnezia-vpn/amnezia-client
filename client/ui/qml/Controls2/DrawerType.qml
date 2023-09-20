import QtQuick
import QtQuick.Controls

Drawer {
    id: drawer
    property bool needCloseButton: true
    property bool isOpened: false

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
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
    }

    onAboutToShow: {
        if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
            PageController.updateNavigationBarColor(0xFF1C1D21)
        }

        if (needCloseButton) {
            PageController.drawerOpen()
        }
    }

    onAboutToHide: {
        if (needCloseButton) {
            PageController.drawerClose()
        }
    }

    onOpened: {
        isOpened = true
    }

    onClosed: {
        isOpened = false

        var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
        if (initialPageNavigationBarColor !== 0xFF1C1D21) {
            PageController.updateNavigationBarColor(initialPageNavigationBarColor)
        }
    }


    onPositionChanged: {
        if (isOpened && (position <= 0.99 && position >= 0.95)) {
            mouseArea.canceled()
            drawer.close()
            mouseArea.exited()
            dropArea.exited()
        }
    }

    DropArea {
        id: dropArea
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent

        onPressed: {
            isOpened = true
        }
    }
}
