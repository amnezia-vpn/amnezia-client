import QtQuick
import QtQuick.Controls

import Style 1.0

Popup {
    id: root

    default property alias sheetData: sheetContent.data

    property real expandedHeight
    readonly property real windowHeight: Overlay.overlay ? Overlay.overlay.height : 0

    property real bottomY: height
    property real closedY: bottomY
    readonly property real openedY: bottomY - Math.min(expandedHeight + contentTopOffset, bottomY)
    readonly property real progress: closedY > openedY ? Math.max(0, Math.min(1, (closedY - sheet.y) / (closedY - openedY))) : 0

    property color defaultColor: AmneziaStyle.color.bgSurfaceAlt
    property color borderColor: AmneziaStyle.color.borderFaint
    property bool showGrabber: true
    property real grabberTopMargin: 8
    readonly property real contentTopOffset: showGrabber ? grabber.anchors.topMargin + grabber.height : 0
    readonly property real navigationBarColor: 0xFF000000
                                               + Math.round(defaultColor.r * 255) * 0x10000
                                               + Math.round(defaultColor.g * 255) * 0x100
                                               + Math.round(defaultColor.b * 255)

    property bool externalDrag: false
    property bool reopenWithParent: false

    function beginDrag() {
        externalDrag = true
        sheet.y = closedY
        open()
    }

    function dragTo(y) {
        sheet.y = Math.max(openedY, Math.min(closedY, y))
    }

    function endDrag() {
        externalDrag = false
        if (sheet.y < closedY) {
            settleAnimation.start()
        } else {
            close()
        }
    }

    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Overlay.overlay.width : 0
    height: windowHeight

    modal: !externalDrag
    dim: false
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape
    background: null

    enter: Transition {
        enabled: !root.externalDrag
        NumberAnimation { target: sheet; property: "y"; from: root.closedY; to: root.openedY; duration: 200 }
    }

    exit: Transition {
        NumberAnimation { target: sheet; property: "y"; to: root.closedY; duration: 200 }
    }

    onOpenedYChanged: {
        if (opened && !externalDrag && !dragArea.drag.active && !settleAnimation.running) {
            sheet.y = openedY
        }
    }

    onOpened: {
        if (!externalDrag && sheet.y !== openedY) {
            settleAnimation.start()
        }
    }

    onAboutToShow: {
        if (PageController.getInitialPageNavigationBarColor() !== root.navigationBarColor) {
            PageController.updateNavigationBarColor(root.navigationBarColor)
        }
    }

    onClosed: {
        externalDrag = false
        const initialPageNavigationBarColor = PageController.getInitialPageNavigationBarColor()
        if (initialPageNavigationBarColor !== root.navigationBarColor) {
            PageController.updateNavigationBarColor(initialPageNavigationBarColor)
        }
    }

    NumberAnimation {
        id: settleAnimation

        target: sheet
        property: "y"
        to: root.openedY
        duration: 200
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive) {
                root.close()
            }
        }
    }

    Connections {
        target: root.parent

        function onVisibleChanged() {
            if (!root.parent.visible && root.opened) {
                root.reopenWithParent = true
                root.close()
            } else if (root.parent.visible && root.reopenWithParent) {
                root.reopenWithParent = false
                root.open()
            }
        }
    }

    contentItem: Item {
        MouseArea {
            anchors.fill: parent

            onClicked: root.close()
        }

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: root.bottomY

            color: AmneziaStyle.color.bgOverlay
            opacity: root.progress
        }

        Item {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: root.bottomY

            clip: true

            Item {
                id: sheet

                width: parent.width
                height: root.bottomY

                Rectangle {
                    anchors.fill: parent
                    anchors.bottomMargin: -border.width

                    topLeftRadius: 16
                    topRightRadius: 16
                    color: root.defaultColor
                    border.color: root.borderColor
                    border.width: 1
                }

                MouseArea {
                    id: dragArea

                    anchors.fill: parent

                    drag.target: sheet
                    drag.axis: Drag.YAxis
                    drag.minimumY: root.openedY
                    drag.maximumY: root.closedY

                    onReleased: {
                        if (sheet.y > root.openedY) {
                            root.close()
                        }
                    }
                }

                SheetGrabberType {
                    id: grabber

                    anchors.top: parent.top
                    anchors.topMargin: root.grabberTopMargin
                    anchors.horizontalCenter: parent.horizontalCenter

                    visible: root.showGrabber
                }

                Flickable {
                    id: sheetFlickable

                    anchors.top: parent.top
                    anchors.topMargin: root.contentTopOffset
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: root.bottomY - root.openedY - root.contentTopOffset

                    contentWidth: width
                    contentHeight: Math.max(height, root.expandedHeight)
                    interactive: contentHeight > height + 1
                    boundsBehavior: Flickable.StopAtBounds
                    clip: interactive

                    ScrollBar.vertical: ScrollBarType {}

                    Item {
                        id: sheetContent

                        width: sheetFlickable.width
                        height: sheetFlickable.contentHeight
                    }
                }
            }
        }
    }
}
