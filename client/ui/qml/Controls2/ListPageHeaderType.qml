import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Item {
    id: root

    required property ListView listView

    property string title
    property Component collapsibleContent

    property alias pinnedContent: pinnedColumn.data
    property alias navigationButtons: navigationBar.actions
    default property alias overlays: overlayLayer.data

    property alias scrollBar: listScrollBar

    readonly property Component listHeader: headerComponent

    readonly property real topBarHeight: topBar.height
    readonly property real pinnedHeight: pinnedBlock.height
    readonly property real pinnedBottom: pinnedBlock.y + pinnedBlock.height

    readonly property real listScrolled: root.listView.contentY - root.listView.originY
    readonly property real collapsibleHeight: root.listView.headerItem ? root.listView.headerItem.collapsibleHeight : 0
    readonly property real collapseProgress: root.collapsibleHeight > 0
                                             ? Math.max(0, Math.min(1, root.listScrolled / root.collapsibleHeight))
                                             : 0
    readonly property real listOcclusion: root.pinnedBottom - root.listView.y
    readonly property bool pinnedStuck: root.listScrolled > 0 && root.listScrolled >= root.collapsibleHeight

    QtObject {
        id: d

        property real lastScrolled: 0
    }

    Connections {
        target: root.listView

        function onContentYChanged() {
            d.lastScrolled = root.listScrolled
        }
    }

    function keepAtTop() {
        if (d.lastScrolled <= 0 && root.listScrolled !== 0) {
            root.listView.contentY = root.listView.originY
        }
    }

    function clampContentY(value) {
        const maxY = root.listView.originY + Math.max(0, root.listView.contentHeight - root.listView.height)
        return Math.max(root.listView.originY, Math.min(value, maxY))
    }

    Component {
        id: headerComponent

        Item {
            readonly property real collapsibleHeight: collapsibleColumn.implicitHeight + collapsibleColumn.anchors.topMargin

            width: root.listView.width
            height: collapsibleHeight + pinnedBlock.height

            onHeightChanged: Qt.callLater(root.keepAtTop)

            ColumnLayout {
                id: collapsibleColumn

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top

                spacing: 0

                opacity: 1 - root.collapseProgress

                Loader {
                    Layout.fillWidth: true

                    sourceComponent: root.collapsibleContent
                }
            }
        }
    }

    Item {
        id: overlayLayer

        anchors.fill: parent
    }

    Rectangle {
        id: topBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        implicitHeight: navigationBar.barHeight

        color: AmneziaStyle.color.bgBase

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        NavigationBarType {
            id: navigationBar

            anchors.left: parent.left
            anchors.right: parent.right
            height: navigationBar.barHeight

            title: root.title
        }
    }

    Rectangle {
        id: pinnedBlock

        anchors.left: parent.left
        anchors.right: parent.right
        y: topBar.height + Math.max(0, root.collapsibleHeight - root.listScrolled)
        height: pinnedColumn.implicitHeight

        color: AmneziaStyle.color.bgBase

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        ColumnLayout {
            id: pinnedColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            spacing: 0
        }
    }

    ScrollBarType {
        id: listScrollBar

        x: root.listView.x + root.listView.width - listScrollBar.width
        y: root.listView.y
        height: root.listView.height
    }
}
