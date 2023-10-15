import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes

Item {
    id: root

    Connections {
        target: PageController

        function onForceCloseDrawer() {
            root.state = "closed"
        }
    }

    visible: false

    signal close()

    property bool needCloseButton: true

    property string defaultColor: "#1C1D21"
    property string borderColor: "#2C2D30"
    property int collapsedHeight: 0

    property bool needCollapsed: false
    property double scaely
    property int contentHeight: 0
    property Item contentParent: contentArea
    // property bool inContentArea: false

    y: parent.height - root.height

    state: "closed"

    Rectangle {
        id: draw2Background

        anchors { left: root.left; right: root.right; top: root.top }
        height: root.parent.height
        width: parent.width
        radius: 16
        color:  "transparent"
        border.color: "transparent" //"green"
        border.width: 1
        visible: true

        Rectangle {
            id: semiArea
            anchors.top: parent.top
            height: parent.height - contentHeight
            anchors.right: parent.right
            anchors.left: parent.left
            visible: true
            color: "transparent"
        }

        Rectangle {
            id: contentArea
            anchors.top: semiArea.bottom
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
        }
    }


    MouseArea {
        id: fullArea
        anchors.fill: parent
        enabled: (root.state === "expanded" || root.state === "opened")
        onClicked: {
            if (root.state === "expanded") {
                root.state = "collapsed"
                return
            }

            if (root.state === "opened") {
                root.state = "closed"
                return
            }
        }
    }

    Drag.active: dragArea.drag.active

    MouseArea {
        id: dragArea

        anchors.fill: parent

        cursorShape: (root.state === "opened"  || root.state === "expanded") ? Qt.PointingHandCursor : Qt.ArrowCursor
        hoverEnabled: true

        drag.target: parent
        drag.axis: Drag.YAxis
        drag.maximumY: parent.height
        drag.minimumY: parent.height - root.height

        /** If drag area is released at any point other than min or max y, transition to the other state */
        onReleased: {
            if (root.state === "closed" && root.y < dragArea.drag.maximumY) {
                root.state = "opened"
                PageController.drawerOpen()
                return
            }

            if (root.state === "opened" && root.y > dragArea.drag.minimumY) {
                root.state = "closed"
                PageController.drawerClose()
                return
            }

            if (root.state === "collapsed" && root.y < dragArea.drag.maximumY) {
                root.state = "expanded"
                return
            }
            if (root.state === "expanded" && root.y > dragArea.drag.minimumY) {
                root.state = "collapsed"
                return
            }
        }

        onClicked: {
            if (root.state === "expanded") {
                root.state = "collapsed"
                return
            }

            if (root.state === "opened") {
                root.state = "closed"
                return
            }
        }
    }

    onStateChanged: {
        if (root.state === "closed" || root.state === "collapsed") {
            var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
            if (initialPageNavigationBarColor !== 0xFF1C1D21) {
                PageController.updateNavigationBarColor(initialPageNavigationBarColor)
            }

            PageController.drawerClose()
            return
        }
        if (root.state === "expanded" || root.state === "opened") {
            if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
                PageController.updateNavigationBarColor(0xFF1C1D21)
            }
            PageController.drawerOpen()
            return
        }
    }

    /** Two states of buttonContent, great place to add any future animations for the drawer */
    states: [
        State {
            name: "collapsed"
            PropertyChanges {
                target: root
                y: root.height - collapsedHeight
            }
        },
        State {
            name: "expanded"
            PropertyChanges {
                target: root
                y: dragArea.drag.minimumY
            }
        },

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
            from: "collapsed"
            to: "expanded"
            PropertyAnimation {
                target: root
                properties: "y"
                duration: 200
            }
        },
        Transition {
            from: "expanded"
            to: "collapsed"
            PropertyAnimation {
                target: root
                properties: "y"
                duration: 200
            }
        },

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

        root.y = 0
        root.state = "opened"
        root.visible = true
        root.height = parent.height
        contentArea.height = contentHeight

        dragArea.drag.maximumY =  parent.height
        dragArea.drag.minimumY =  parent.height - root.height


        animationVisible.running = true

        if (needCloseButton) {
            PageController.drawerOpen()
        }

        if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
            PageController.updateNavigationBarColor(0xFF1C1D21)
        }
    }

    function onClose() {
        if (needCloseButton) {
            PageController.drawerClose()
        }

        var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
        if (initialPageNavigationBarColor !== 0xFF1C1D21) {
            PageController.updateNavigationBarColor(initialPageNavigationBarColor)
        }

        root.visible = false
    }

    onVisibleChanged: {
        if (!visible) {
            close()
        }
    }
}
