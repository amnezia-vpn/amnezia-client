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

    width: parent.width - leftMargin - rightMargin

    x: (parent ? (parent.width - width) / 2 : 0)
    y: 16 + SettingsController.safeAreaTopMargin
    modal: root.closeButtonVisible
    closePolicy: Popup.CloseOnEscape

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 }
    }

    Overlay.modal: Rectangle {
        visible: root.closeButtonVisible
        color: FBLinkStyle.color.translucentMidnightBlack
    }

    onOpened: {
        timer.start()
    }

    onClosed: {
        FocusController.dropRootObject(root)
    }

    background: Rectangle {
        anchors.fill: parent

        color: FBLinkStyle.color.onyxBlack
        radius: 14
        border.width: 1
        border.color: Qt.rgba(234/255, 179/255, 8/255, 0.35)
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
            anchors.topMargin: 10
            anchors.bottomMargin: 10

            CaptionTextType {
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
                color: FBLinkStyle.color.paleGray

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(LanguageModel.getCurrentDocsUrl(link))
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

                defaultColor: Qt.rgba(234/255, 179/255, 8/255, 0.18)
                hoveredColor: Qt.rgba(234/255, 179/255, 8/255, 0.26)
                pressedColor: Qt.rgba(234/255, 179/255, 8/255, 0.34)
                disabledColor: FBLinkStyle.color.charcoalGray

                textColor: FBLinkStyle.color.paleGray
                borderWidth: 0

                text: qsTr("Close")

                clickedFunc: function() {
                    root.close()
                }
            }
        }
    }
}
