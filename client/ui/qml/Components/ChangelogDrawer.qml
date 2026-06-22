import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

import "../Config"

DrawerType2 {
    id: root

    anchors.fill: parent
    expandedHeight: parent.height * 0.9

    expandedStateContent: Item {
        implicitHeight: root.expandedHeight

        Header2TextType {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16
            anchors.rightMargin: 16
            anchors.leftMargin: 16
            anchors.bottomMargin: 16

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
                anchors.topMargin: 16
                anchors.rightMargin: 16
                anchors.leftMargin: 16
                anchors.bottomMargin: 16

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

        BasicButtonType {
            id: updateButton
            anchors.bottom: skipButton.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: 8
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            text: qsTr("Update")

            clickedFunc: function() {
                PageController.showBusyIndicator(true)
                UpdateController.runInstaller()
                PageController.showBusyIndicator(false)
                root.closeTriggered()
            }
        }

        BasicButtonType {
            id: skipButton
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: 16
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            defaultColor: AmneziaStyle.color.transparent
            hoveredColor: AmneziaStyle.color.translucentWhite
            pressedColor: AmneziaStyle.color.sheerWhite
            disabledColor: AmneziaStyle.color.mutedGray
            textColor: AmneziaStyle.color.paleGray
            borderWidth: 1

            text: qsTr("Skip")

            clickedFunc: function() {
                root.closeTriggered()
            }
        }
    }
}
