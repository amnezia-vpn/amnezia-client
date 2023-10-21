import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

Item {
    id: root

    Connections {
        target: PageController

        function onForceCloseDrawer() {
            if (root.expanded()) {
                collapse()
            }
        }
    }

    signal drawerClosed
    signal collapsedEntered
    signal collapsedExited
    signal collapsedEnter
    signal collapsedPressChanged


    visible: false

    property bool needCloseButton: true

    property string defaultColor: "#1C1D21"
    property string borderColor: "#2C2D30"
    property string semitransparentColor: "#90000000"

    property bool needCollapsed: false

    property int contentHeight: 0
    property Item contentParent: contentArea

    property bool dragActive: dragArea.drag.active

    property int collapsedHeight: 0

    property bool fullMouseAreaVisible: true
    property MouseArea drawerDragArea: dragArea

    state: "collapsed"

    Rectangle {
        id: draw2Background

        anchors.fill: parent
        height: parent.height
        width: parent.width
        radius: 16
        color: "transparent"
        border.color: "transparent"
        border.width: 1
        visible: true

        MouseArea {
            id: fullMouseArea
            anchors.fill: parent
            enabled: root.expanded()
            hoverEnabled: true
            visible: fullMouseAreaVisible

            onClicked: {
                if (root.expanded()) {
                    collapse()
                }
            }
        }

        Rectangle {
            id: placeAreaHolder

            // for apdating home drawer, normal drawer will reset it
            height: 0

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

                cursorShape: root.collapsed() ? Qt.PointingHandCursor : Qt.ArrowCursor
                hoverEnabled: true

                drag.target: placeAreaHolder
                drag.axis: Drag.YAxis
                drag.maximumY: root.height - root.collapsedHeight
                drag.minimumY: root.collapsedHeight > 0 ? root.height - root.height * 0.9 : 0

                /** If drag area is released at any point other than min or max y, transition to the other state */
                onReleased: {
                    if (root.collapsed() && placeAreaHolder.y < drag.maximumY) {
                        root.state = "expanded"
                        return
                    }
                    if (root.expanded() && placeAreaHolder.y > drag.minimumY) {
                        root.state = "collapsed"
                        return
                    }
                }

                onClicked: {
                    if (root.expanded()) {
                        collapse()
                        return
                    }

                    if (root.collapsed()) {
                        root.state = "expanded"
                    }
                }

                onExited: {
                    collapsedExited()
                }

                onEntered: {
                    collapsedEnter()
                }

                onPressedChanged: {
                    collapsedPressChanged()
                }
            }
        }
    }

    onStateChanged: {
        if (root.collapsed()) {
            var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
            if (initialPageNavigationBarColor !== 0xFF1C1D21) {
                PageController.updateNavigationBarColor(initialPageNavigationBarColor)
            }

            if (needCloseButton) {
                PageController.drawerClose()
            }

            drawerClosed()

            return
        }

        if (root.expanded()) {
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
            name: "collapsed"
            PropertyChanges {
                target: placeAreaHolder
                y: dragArea.drag.maximumY
            }
        },

        State {
            name: "expanded"
            PropertyChanges {
                target: placeAreaHolder
                y: dragArea.drag.minimumY
            }
        }
    ]

    transitions: [
        Transition {
            from: "expanded"
            to: "collapsed"
            PropertyAnimation {
                target: placeAreaHolder
                properties: "y"
                duration: 200
            }

            onRunningChanged: {
                if (!running) {
                    draw2Background.color = "transparent"
                    fullMouseArea.visible = false
                }
            }
        },

        Transition {
            from: "collapsed"
            to: "expanded"
            PropertyAnimation {
                target: placeAreaHolder
                properties: "y"
                duration: 200
            }

            onRunningChanged: {
                if (!running) {
                    draw2Background.color = semitransparentColor
                    fullMouseArea.visible = true
                }
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

    // for normal drawer
    function open() {
        if (root.expanded()) {
            return
        }

        draw2Background.color = semitransparentColor
        fullMouseArea.visible = true

        collapsedHeight = 0

        root.y = 0
        root.state = "expanded"
        root.visible = true
        root.height = parent.height

        contentArea.height = contentHeight

        placeAreaHolder.y = 0
        placeAreaHolder.height = root.height - contentHeight

        animationVisible.running = true
    }

    function close() {
        collapse()
    }

    function collapse() {
        draw2Background.color = "transparent"
        root.state = "collapsed"
    }

    // for page home
    function expand() {
        draw2Background.color = semitransparentColor
        root.state = "expanded"
    }

    function expanded() {
        return root.state === "expanded" ? true : false
    }

    function collapsed() {
        return root.state === "collapsed" ? true : false
    }


    onVisibleChanged: {
        // e.g cancel, ......
        if (!visible) {
            if (root.expanded()) {
                if (needCloseButton) {
                    PageController.drawerClose()
                }

                close()
            }
        }
    }
}
