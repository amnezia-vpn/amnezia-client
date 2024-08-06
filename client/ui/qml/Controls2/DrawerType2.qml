import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Config"
import "TextTypes"

FocusScope {
    id: root

    /*required*/ property var parentPage

    readonly property string drawerExpanded: "expanded"
    readonly property string drawerCollapsed: "collapsed"

    readonly property bool isOpened: drawerContent.state === root.drawerExpanded || (drawerContent.state === root.drawerCollapsed && dragArea.drag.active === true)
    readonly property bool isClosed: drawerContent.state === root.drawerCollapsed && dragArea.drag.active === false

    readonly property bool isExpanded: drawerContent.state === root.drawerExpanded
    readonly property bool isCollapsed: drawerContent.state === root.drawerCollapsed

    property Component collapsedContent
    property Component expandedContent

    property string defaultColor: AmneziaStyle.color.blackLight
    property string borderColor: AmneziaStyle.color.greyDark

    property real expandedHeight
    property real collapsedHeight: 0

    property int depthIndex: 0

    signal entered
    signal exited
    signal pressed(bool pressed, bool entered)

    signal aboutToHide
    signal aboutToShow
    signal close
    signal open
    signal closed
    signal opened

    onClosed: {
        console.log("Drawer closed============================")
        if (GC.isMobile()) {
            return
        }

        if (parentPage) {
            parentPage.forceActiveFocus()
        }
        // PageController.forceStackActiveFocus()
    }

    onOpened: {
        console.log("Drawer opened============================")
        if (GC.isMobile()) {
            return
        }

        if (root.expandedContent) {
            expandedLoader.item.forceActiveFocus()
        }
    }

    onActiveFocusChanged: {
        console.log("Drawer active focus changed============================")
        if (GC.isMobile()) {
            return
        }

        if (root.activeFocus && !root.isOpened && root.collapsedContent) {
            collapsedLoader.item.forceActiveFocus()
        }
    }

    // onOpened: {
    //     if (isOpened) {
    //         if (drawerContent.state === root.drawerExpanded) {
    //             expandedLoader.item.forceActiveFocus()
    //         } else {
    //             collapsedLoader.item.forceActiveFocus()
    //         }
    //     }
    // }

    Connections {
        target: PageController

        function onCloseTopDrawer() {
            if (depthIndex === PageController.getDrawerDepth()) {
                if (isCollapsed) {
                    return
                }

                aboutToHide()

                drawerContent.state = root.drawerCollapsed
                depthIndex = 0
                closed()
            }
        }
    }

    Connections {
        target: root

        function onClose() {
            if (isCollapsed) {
                return
            }

            aboutToHide()

            drawerContent.state = root.drawerCollapsed
            depthIndex = 0
            PageController.setDrawerDepth(PageController.getDrawerDepth() - 1)
            closed()
        }

        function onOpen() {
            if (isExpanded) {
                return
            }

            aboutToShow()

            drawerContent.state = root.drawerExpanded
            depthIndex = PageController.getDrawerDepth() + 1
            PageController.setDrawerDepth(depthIndex)
            opened()
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: root.isCollapsed ? AmneziaStyle.color.transparent : Qt.rgba(14/255, 14/255, 17/255, 0.8)

        Behavior on color {
            PropertyAnimation { duration: 200 }
        }
    }

    MouseArea {
        id: emptyArea
        anchors.fill: parent
        enabled: root.isExpanded
        visible: enabled
        onClicked: {
            root.close()
        }
    }

    MouseArea {
        id: dragArea

        anchors.fill: drawerContentBackground
        cursorShape: root.isCollapsed ? Qt.PointingHandCursor : Qt.ArrowCursor
        hoverEnabled: true

        enabled: drawerContent.implicitHeight > 0

        drag.target: drawerContent
        drag.axis: Drag.YAxis
        drag.maximumY: root.height - root.collapsedHeight
        drag.minimumY: root.height - root.expandedHeight

        /** If drag area is released at any point other than min or max y, transition to the other state */
        onReleased: {
            if (root.isCollapsed && drawerContent.y < dragArea.drag.maximumY) {
                root.open()
                return
            }
            if (root.isExpanded && drawerContent.y > dragArea.drag.minimumY) {
                root.close()
                return
            }
        }

        onEntered: {
            root.entered()
        }
        onExited: {
            root.exited()
        }
        onPressedChanged: {
            root.pressed(pressed, entered)
        }

        onClicked: {
            if (root.isCollapsed) {
                root.open()
            }
        }
    }

    Rectangle {
        id: drawerContentBackground

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

        focus: true
        Drag.active: dragArea.drag.active
        anchors.right: root.right
        anchors.left: root.left
        y: root.height - drawerContent.height
        state: root.drawerCollapsed

        implicitHeight: root.isCollapsed ? collapsedHeight : expandedHeight

        onStateChanged: {
            if (root.isCollapsed) {
                var initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
                if (initialPageNavigationBarColor !== 0xFF1C1D21) {
                    PageController.updateNavigationBarColor(initialPageNavigationBarColor)
                }
                return
            }
            if (root.isExpanded) {
                if (PageController.getInitialPageNavigationBarColor() !== 0xFF1C1D21) {
                    PageController.updateNavigationBarColor(0xFF1C1D21)
                }
                return
            }
        }

        states: [
            State {
                name: root.drawerCollapsed
                PropertyChanges {
                    target: drawerContent
                    y: root.height - root.collapsedHeight
                }
            },
            State {
                name: root.drawerExpanded
                PropertyChanges {
                    target: drawerContent
                    y: dragArea.drag.minimumY

                }
            }
        ]

        transitions: [
            Transition {
                from: root.drawerCollapsed
                to: root.drawerExpanded
                PropertyAnimation {
                    target: drawerContent
                    properties: "y"
                    duration: 200
                }
            },
            Transition {
                from: root.drawerExpanded
                to: root.drawerCollapsed
                PropertyAnimation {
                    target: drawerContent
                    properties: "y"
                    duration: 200
                }
            }
        ]

        Loader {
            id: collapsedLoader

            sourceComponent: root.collapsedContent

            anchors.right: parent.right
            anchors.left: parent.left
        }

        Loader {
            id: expandedLoader

            visible: root.isExpanded
            sourceComponent: root.expandedContent

            anchors.right: parent.right
            anchors.left: parent.left
        }
    }
}
