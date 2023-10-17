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

    y: parent.height - root.height

    state: "closed"

    Rectangle {
        id: draw2Background

        anchors { left: root.left; right: root.right; top: root.top }
        height: root.parent.height
        width: parent.width
        radius: 16
        color:  "#90000000"
        border.color: "transparent"
        border.width: 1
        visible: true

        MouseArea {
            id: fullArea
            anchors.fill: parent
            enabled: (root.state === "opened")
            hoverEnabled: true

            onClicked: {
                if (root.state === "opened") {
                    draw2Background.color = "transparent"
                    root.state = "closed"
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
                        if (root.state === "closed" && placeAreaHolder.y < dragArea.drag.maximumY) {
                            root.state = "opened"
                            return
                        }

                        if (root.state === "opened" && placeAreaHolder.y > dragArea.drag.minimumY) {
                            close()
                            return
                        }
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
                target: root
                y: parent.height
            }
        },

        State {
            name: "opend"
            PropertyChanges {
                target: root
                y: dragArea.drag.minimumY
            }
        }
    ]

    transitions: [
        Transition {
            from: "opened"
            to: "closed"
            PropertyAnimation {
                target: root
                properties: "y"
                duration: 200
            }
        },

        Transition {
            from: "closed"
            to: "opened"
            PropertyAnimation {
                target: root
                properties: "y"
                duration: 200
            }
        }
    ]

    NumberAnimation {
        id: animationVisible
        target: root
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

        root.y = 0
        root.state = "opened"
        root.visible = true
        root.height = parent.height
        contentArea.height = contentHeight
        placeAreaHolder.height =  parent.height - contentHeight
        placeAreaHolder.y = parent.height - root.height

        dragArea.drag.maximumY =  parent.height
        dragArea.drag.minimumY =  parent.height - root.height

        animationVisible.running = true
    }

    function close() {
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
