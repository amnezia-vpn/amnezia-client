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

        color: AmneziaStyle.color.bgBase

        implicitHeight: navigationRow.y + navigationRow.height

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        Item {
            id: navigationRow

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: PageController.safeAreaTopMargin
            height: 56

            IconButtonType {
                id: backButton

                x: 4
                y: 4

                icon.source: "qrc:/images/controls/arrow-left.svg"
                Accessible.name: qsTr("Back")

                onClicked: PageController.closePage()
            }

            H3TextType {
                readonly property real sideWidth: Math.max(48, navigationButtonsRow.width)

                x: 4 + sideWidth
                y: (navigationRow.height - height) / 2
                width: navigationRow.width - 8 - 2 * sideWidth

                font.weight: 600
                font.letterSpacing: 0

                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.NoWrap
                maximumLineCount: 1
                elide: Text.ElideRight
                text: root.title
            }

            Row {
                id: navigationButtonsRow

                anchors.right: parent.right
                anchors.rightMargin: 4
                y: 4
            }
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
