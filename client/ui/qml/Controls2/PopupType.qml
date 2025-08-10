import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"
import "../Config"

Popup {
    id: root

    property string text
    property bool closeButtonVisible: true

    leftMargin: 25
    rightMargin: 25
    bottomMargin: 70

    width: parent.width - leftMargin - rightMargin

    anchors.centerIn: parent
    modal: root.closeButtonVisible
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle {
        visible: root.closeButtonVisible
        color: AmneziaStyle.color.translucentMidnightBlack
    }

    onOpened: {
        timer.start()
    }

    onClosed: {
        FocusController.dropRootObject(root)
    }

    background: Rectangle {
        anchors.fill: parent

        color: "white"
        radius: 4
    }

    Timer {
        id: timer
        interval: 200 // Milliseconds
        onTriggered: {
            FocusController.pushRootObject(root)
            FocusController.setFocusItem(closeButton)
        }
        repeat: false // Stop the timer after one trigger
        running: true // Start the timer
    }

    contentItem: Item {
        implicitWidth: content.implicitWidth
        implicitHeight: content.implicitHeight

        anchors.fill: parent

        RowLayout {
            id: content

            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            CaptionTextType {
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                text: root.text

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            BasicButtonType {
                id: closeButton
                visible: closeButtonVisible

                implicitHeight: 32

                defaultColor: "white"
                hoveredColor: AmneziaStyle.color.lightGray
                pressedColor: AmneziaStyle.color.lightGray
                disabledColor: AmneziaStyle.color.charcoalGray

                textColor: AmneziaStyle.color.midnightBlack
                borderWidth: 0

                text: qsTr("Close")

                clickedFunc: function() {
                    root.close()
                }
            }
        }
    }
}
