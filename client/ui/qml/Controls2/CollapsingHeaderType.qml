import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    required property ListView listView

    property string title
    property Component collapsibleContent

    property alias pinnedContent: pinnedColumn.data
    property alias navigationButtons: navigationButtonsRow.data
    default property alias overlays: overlayLayer.data

    property alias backButton: backButton
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
    readonly property bool pinnedStuck: root.listScrolled >= root.collapsibleHeight

    function clampContentY(value) {
        const maxY = root.listView.originY + Math.max(0, root.listView.contentHeight - root.listView.height)
        return Math.max(root.listView.originY, Math.min(value, maxY))
    }

    Component {
        id: headerComponent

        Item {
            readonly property real collapsibleHeight: collapsibleColumn.implicitHeight + 4

            width: root.listView.width
            height: collapsibleHeight + pinnedBlock.height

            ColumnLayout {
                id: collapsibleColumn

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 4

                spacing: 0

                opacity: 1 - root.collapseProgress

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: largeTitleMetrics.height
                }

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

        color: AmneziaStyle.color.midnightBlack

        implicitHeight: navigationRow.y + navigationRow.height + 4

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        Item {
            id: navigationRow

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 20 + PageController.safeAreaTopMargin
            height: backButton.implicitHeight

            BackButtonType {
                id: backButton
                objectName: "backButton"

                anchors.left: parent.left
                anchors.right: navigationButtonsRow.left
                anchors.verticalCenter: parent.verticalCenter
            }

            Row {
                id: navigationButtonsRow

                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Rectangle {
        id: pinnedBlock

        anchors.left: parent.left
        anchors.right: parent.right
        y: topBar.height + Math.max(0, root.collapsibleHeight - root.listScrolled)
        height: pinnedColumn.implicitHeight

        color: AmneziaStyle.color.midnightBlack

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

    Header1TextType {
        id: largeTitleMetrics

        visible: false
        text: movingTitle.text
    }

    Header1TextType {
        id: movingTitle

        readonly property real compactScale: 18 / 32
        readonly property real titleScale: 1 - (1 - movingTitle.compactScale) * root.collapseProgress
        readonly property real expandedX: 16
        readonly property real compactX: (root.width - movingTitle.width * movingTitle.compactScale) / 2
        readonly property real expandedY: topBar.height + 4 - Math.min(0, root.listScrolled)
        readonly property real compactY: navigationRow.y
                                         + (navigationRow.height - movingTitle.implicitHeight * movingTitle.compactScale) / 2

        x: movingTitle.expandedX + (movingTitle.compactX - movingTitle.expandedX) * root.collapseProgress
        y: Math.max(movingTitle.compactY, movingTitle.expandedY - Math.max(0, root.listScrolled))
        width: Math.min(movingTitle.implicitWidth, root.width - 32)

        transformOrigin: Item.TopLeft
        scale: movingTitle.titleScale

        wrapMode: Text.NoWrap
        maximumLineCount: 1
        elide: Text.ElideRight
        text: root.title
    }

    ScrollBarType {
        id: listScrollBar

        x: root.listView.x + root.listView.width - listScrollBar.width
        y: root.listView.y
        height: root.listView.height
    }
}
