import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

Item {
    id: root

    Connections {
        target: PageController

        function onForceCloseDrawer() {
            close()
        }
    }

    signal closed

    visible: false

    property bool needCloseButton: true

    property string defaultColor: "#1C1D21"
    property string borderColor: "#2C2D30"

    property bool needCollapsed: false

    property int contentHeight: 0
    property Item contentParent: contentArea

    state: "closed"

    Rectangle {
        id: draw2Background

        anchors.fill: parent
        height: parent.height
        width: parent.width
        radius: 16
        color:  "#90000000"
        border.color: "transparent"
        border.width: 1
        visible: true

        MouseArea {
            id: fullMouseArea
            anchors.fill: parent
            enabled: (root.state === "opened")
            hoverEnabled: true

            onClicked: {
                if (root.state === "opened") {
                    close()
                    return
                }
            }

            Rectangle {
                id: placeAreaHolder
                height: parent.height - contentHeight
                anchors.right: parent.right
                anchors.left: parent.left
                visible: true
                color: "transparent"

                Drag.active: dragArea.drag.active
            }


            Rectangle {
                id: contentArea

                anchors.top: placeAreaHolder.bottom
                height: contentHeight
                radius: 16
                color: root.defaultColor
                border.width: 1
                border.color: root.borderColor
                width: parent.width
                visible: true

                Rectangle {
                    width: parent.radius
                    height: parent.radius
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.left: parent.left
                    color: parent.color
                }

                MouseArea {
                    id: dragArea

                    anchors.fill: parent

                    cursorShape: (root.state === "opened") ? Qt.PointingHandCursor : Qt.ArrowCursor
                    hoverEnabled: true

                    drag.target: placeAreaHolder
                    drag.axis: Drag.YAxis
                    drag.maximumY: root.height
                    drag.minimumY: root.height - root.height

                    /** If drag area is released at any point other than min or max y, transition to the other state */
                    onReleased: {
                        if (root.state === "closed" && placeAreaHolder.y < root.height * 0.9) {
                            root.state = "opened"
                            return
                        }

                        if (root.state === "opened" && placeAreaHolder.y > (root.height - root.height * 0.9)) {
                            close()
                            return
                        }

                        placeAreaHolder.y = 0
                    }

                    onClicked: {
                        if (root.state === "opened") {
                            close()
                            return
                        }
                    }
                }
            }
        }
    }

    onStateChanged: {
        if (root.state === "closed") {
            var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
            if (initialPageNavigationBarColor !== 0xFF1C1D21) {
                PageController.updateNavigationBarColor(initialPageNavigationBarColor)
            }

            if (needCloseButton) {
                PageController.drawerClose()
            }

            closed()

            return
        }
        if (root.state === "opened") {
            if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
                PageController.updateNavigationBarColor(0xFF1C1D21)
            }

            if (needCloseButton) {
                PageController.drawerOpen()
            }

            return
        }
    }

    /** Two states of buttonContent, great place to add any future animations for the drawer */
    states: [
        State {
            name: "closed"
            PropertyChanges {
                target: placeAreaHolder
                y: parent.height
            }
        },

        State {
            name: "opened"
            PropertyChanges {
                target: placeAreaHolder
                y: dragArea.drag.minimumY
            }
        }
    ]

    transitions: [
        Transition {
            from: "opened"
            to: "closed"
            PropertyAnimation {
                target: placeAreaHolder
                properties: "y"
                duration: 200
            }
        },

        Transition {
            from: "closed"
            to: "opened"
            PropertyAnimation {
                target: placeAreaHolder
                properties: "y"
                duration: 200
            }
        }
    ]

    NumberAnimation {
        id: animationVisible
        target: placeAreaHolder
        property: "y"
        from: parent.height
        to: 0
        duration: 200
    }

    function open() {
        if (root.visible && root.state !== "closed") {
            return
        }
        draw2Background.color = "#90000000"

        fullMouseArea.visible = true
        dragArea.visible = true

        root.y = 0
        root.state = "opened"
        root.visible = true
        root.height = parent.height

        contentArea.height = contentHeight

        placeAreaHolder.height =  root.height - contentHeight
        placeAreaHolder.y = root.height - root.height

        dragArea.drag.maximumY =  root.height
        dragArea.drag.minimumY =  0

        animationVisible.running = true
    }

    function close() {
        fullMouseArea.visible = false
        dragArea.visible = false

        draw2Background.color = "transparent"
        root.state = "closed"
    }

    onVisibleChanged: {
        // e.g cancel, ......
        if (!visible) {
            if (root.state === "opened") {
                if (needCloseButton) {
                    PageController.drawerClose()
                }
            }

            close()
        }
    }
}
