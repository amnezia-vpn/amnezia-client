import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    readonly property string drawerExpandedStateName: "expanded"
    readonly property string drawerCollapsedStateName: "collapsed"

    readonly property bool isOpened: isExpandedStateActive() || (isCollapsedStateActive() && (dragArea.drag.active === true))
    readonly property bool isClosed: isCollapsedStateActive() && (dragArea.drag.active === false)

    property Component collapsedStateContent
    property Component expandedStateContent

    property string defaultColor: AmneziaStyle.color.onyxBlack
    property string borderColor: AmneziaStyle.color.slateGray

    property real expandedHeight
    property real collapsedHeight: 0

    property int depthIndex: 0

    signal cursorEntered
    signal cursorExited
    signal pressed(bool pressed, bool entered)

    signal aboutToHide
    signal aboutToShow
    signal closeTriggered
    signal openTriggered
    signal closed
    signal opened

    function isExpandedStateActive() {
        return isStateActive(drawerExpandedStateName)
    }

    function isCollapsedStateActive() {
        return isStateActive(drawerCollapsedStateName)
    }

    function isStateActive(stateName) {
        return drawerContent.state === stateName
    }

    function isDrawerType2(obj) {
        return obj && typeof obj.drawerExpandedStateName !== "undefined" && 
               typeof obj.drawerCollapsedStateName !== "undefined"
    }

    function isDescendantOfDrawer(obj) {
        var current = obj
        while (current && current !== root.parent) {
            if (isDrawerType2(current)) {
                return true
            }
            current = current.parent
        }
        return false
    }

    function findComponent(obj, typeCtor) {
        if (!obj)
            return null

        if (isDrawerType2(obj) || isDescendantOfDrawer(obj))
            return null

        if (obj instanceof typeCtor)
            return obj

        if (obj.children && obj.children.length > 0) {
            for (var i = 0; i < obj.children.length; i++) {
                var matchingChildren = findComponent(obj.children[i], typeCtor)
                if (matchingChildren) return matchingChildren
            }
        }

        if (obj.contentItem) {
            var matchingContentItem = findComponent(obj.contentItem, typeCtor)
            if (matchingContentItem) return matchingContentItem
        }

        return null
    }

    function setParentInteractive(value) {
        var flickableType = findComponent(root.parent, Flickable)
        var listViewType = findComponent(root.parent, ListView)

        if (flickableType) flickableType.interactive = value
        if (listViewType) listViewType.interactive = value
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive) {
                if (dragArea.drag.active) {
                    dragArea.drag.target = null
                    dragArea.drag.target = drawerContent
                }
                if (isOpened && !isCollapsedStateActive()) {
                    root.closeTriggered()
                }
            }
        }
    }

    Connections {
        target: PageController

        function onCloseTopDrawer() {
            if (depthIndex === PageController.getDrawerDepth()) {
                if (isCollapsedStateActive()) {
                    return
                }

                aboutToHide()

                drawerContent.state = root.drawerCollapsedStateName
                depthIndex = 0
                closed()
            }
        }
    }

    Connections {
        target: root

        function onCloseTriggered() {
            if (isCollapsedStateActive()) {
                return
            }

            aboutToHide()

            setParentInteractive(true)

            closed()
        }

        function onClosed() {
            drawerContent.state = root.drawerCollapsedStateName
            
            if (root.isCollapsedStateActive()) {
                var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
                if (initialPageNavigationBarColor !== 0xFF1C1D21) {
                    PageController.updateNavigationBarColor(initialPageNavigationBarColor)
                }
            }

            depthIndex = 0
            PageController.decrementDrawerDepth()
            FocusController.dropRootObject(root)
        }

        function onOpenTriggered() {
            if (root.isExpandedStateActive()) {
                return
            }

            root.aboutToShow()

            setParentInteractive(false)

            root.opened()
        }

        function onOpened() {
            drawerContent.state = root.drawerExpandedStateName

            if (isExpandedStateActive()) {
                if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
                    PageController.updateNavigationBarColor(0xFF1C1D21)
                }
            }

            depthIndex = PageController.incrementDrawerDepth()
            FocusController.pushRootObject(root)
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: root.isCollapsedStateActive() ? AmneziaStyle.color.transparent : AmneziaStyle.color.translucentMidnightBlack

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    MouseArea {
        id: emptyArea
        anchors.fill: parent

        onClicked: {
            root.closeTriggered()
        }
    }

    MouseArea {
        id: dragArea
        objectName: "dragArea"

        anchors.fill: drawerContentBackground
        hoverEnabled: true

        enabled: drawerContent.implicitHeight > 0

        drag.target: drawerContent
        drag.axis: Drag.YAxis
        drag.maximumY: root.height - root.collapsedHeight
        drag.minimumY: root.height - root.expandedHeight

        /** If drag area is released at any point other than min or max y, transition to the other state */
        onReleased: {
            if (isCollapsedStateActive() && drawerContent.y < dragArea.drag.maximumY) {
                root.openTriggered()
                return
            }
            if (isExpandedStateActive() && drawerContent.y > dragArea.drag.minimumY) {
                root.closeTriggered()
                return
            }
        }

        onEntered: {
            root.cursorEntered()
        }
        onExited: {
            root.cursorExited()
        }
        onPressedChanged: {
            root.pressed(pressed, entered)
        }

        onClicked: {
            if (isCollapsedStateActive()) {
                root.openTriggered()
            }
        }
    }

    Rectangle {
        id: drawerContentBackground
        objectName: "drawerContentBackground"

        anchors { left: drawerContent.left; right: drawerContent.right; top: drawerContent.top }
        height: root.height
        radius: 16
        color: root.defaultColor
        border.color: root.borderColor
        border.width: 1

        Rectangle {
            width: parent.radius
            height: parent.radius
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            color: parent.color
        }
    }

    Item {
        id: drawerContent
        objectName: "drawerContent"

        Drag.active: dragArea.drag.active
        anchors.right: root.right
        anchors.left: root.left

        state: root.drawerCollapsedStateName

        states: [
            State {
                name: root.drawerCollapsedStateName
                PropertyChanges {
                    target: drawerContent
                    implicitHeight: collapsedHeight
                    y: root.height - root.collapsedHeight
                }
                PropertyChanges {
                    target: background
                    color: AmneziaStyle.color.transparent
                }
                PropertyChanges {
                    target: dragArea
                    cursorShape: Qt.PointingHandCursor
                }
                PropertyChanges {
                    target: emptyArea
                    enabled: false
                    visible: false
                }
                PropertyChanges {
                    target: collapsedLoader
                    // visible: true
                }
                PropertyChanges {
                    target: expandedLoader
                    visible: false

                }
            },
            State {
                name: root.drawerExpandedStateName
                PropertyChanges {
                    target: drawerContent
                    implicitHeight: expandedHeight
                    y: dragArea.drag.minimumY
                }
                PropertyChanges {
                    target: background
                    color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
                }
                PropertyChanges {
                    target: dragArea
                    cursorShape: Qt.ArrowCursor
                }
                PropertyChanges {
                    target: emptyArea
                    enabled: true
                    visible: true
                }
                PropertyChanges {
                    target: collapsedLoader
                    // visible: false
                }
                PropertyChanges {
                    target: expandedLoader
                    visible: true
                }
            }
        ]

        transitions: [
            Transition {
                from: root.drawerCollapsedStateName
                to: root.drawerExpandedStateName
                PropertyAnimation {
                    target: drawerContent
                    properties: "y"
                    duration: 200
                }
            },
            Transition {
                from: root.drawerExpandedStateName
                to: root.drawerCollapsedStateName
                PropertyAnimation {
                    target: drawerContent
                    properties: "y"
                    duration: 200
                }
            }
        ]

        Loader {
            id: collapsedLoader

            sourceComponent: root.collapsedStateContent

            anchors.right: parent.right
            anchors.left: parent.left
        }

        Loader {
            id: expandedLoader

            sourceComponent: root.expandedStateContent

            anchors.right: parent.right
            anchors.left: parent.left
        }
    }
}
