import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"
import "../Controls2/TextTypes"

import "../Config"

DrawerType2 {
    id: root

    anchors.fill: parent
    expandedHeight: parent.height * 0.9

    expandedContent: Item {
        implicitHeight: root.expandedHeight

        Connections {
            target: root
            enabled: !GC.isMobile()
            function onOpened() {
                focusItem.forceActiveFocus()
            }
        }

        Header2TextType {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            text: UpdateController.headerText
        }

        FlickableType {
            anchors.top: header.bottom
            anchors.bottom: updateButton.top
            contentHeight: changelog.height + 32

            ParagraphTextType {
                id: changelog
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 48
                anchors.rightMargin: 16
                anchors.leftMargin: 16

                HoverHandler {
                    enabled: parent.hoveredLink
                    cursorShape: Qt.PointingHandCursor
                }

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                text: UpdateController.changelogText
                textFormat: Text.MarkdownText
            }
        }

        Item {
            id: focusItem
            KeyNavigation.tab: updateButton
        }

        BasicButtonType {
            id: updateButton
            anchors.bottom: skipButton.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16
            anchors.bottomMargin: 8
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            text: qsTr("Update")

            clickedFunc: function() {
                PageController.showBusyIndicator(true)
                UpdateController.runInstaller()
                PageController.showBusyIndicator(false)
                root.close()
            }

            KeyNavigation.tab: skipButton
        }

        BasicButtonType {
            id: skipButton
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: 16
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            defaultColor: "transparent"
            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
            pressedColor: Qt.rgba(1, 1, 1, 0.12)
            disabledColor: "#878B91"
            textColor: "#D7D8DB"
            borderWidth: 1

            text: qsTr("Skip this version")

            clickedFunc: function() {
                root.close()
            }

            KeyNavigation.tab: focusItem
        }
    }
}
