import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Popup {
    id: root

    property string text
    property string actionText
    property int duration: 4000

    signal actionTriggered()

    function show(message, action) {
        root.text = message
        root.actionText = action || ""
        swipe.offsetX = 0
        swipe.offsetY = 0
        root.open()
        hideTimer.restart()
    }

    parent: Overlay.overlay

    width: Math.min(parent.width - 32, 520)
    x: (parent.width - width) / 2
    y: parent.height - height - 24 - PageController.safeAreaBottomMargin - PageController.imeHeight

    padding: 0
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose

    onClosed: hideTimer.stop()

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.surfaceInverse

        transform: Translate {
            x: swipe.offsetX
            y: swipe.offsetY
        }
    }

    contentItem: Item {
        id: swipe

        property real offsetX: 0
        property real offsetY: 0

        implicitHeight: content.implicitHeight + 32

        transform: Translate {
            x: swipe.offsetX
            y: swipe.offsetY
        }

        DragHandler {
            id: swipeHandler

            target: null

            onActiveTranslationChanged: {
                swipe.offsetX = swipeHandler.activeTranslation.x
                swipe.offsetY = Math.max(0, swipeHandler.activeTranslation.y)
            }

            onActiveChanged: {
                if (swipeHandler.active) {
                    hideTimer.stop()
                    return
                }
                if (Math.abs(swipe.offsetX) > 60 || swipe.offsetY > 40) {
                    root.close()
                    return
                }
                swipe.offsetX = 0
                swipe.offsetY = 0
                hideTimer.restart()
            }
        }

        RowLayout {
            id: content

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            spacing: 16

            ParagraphTextType {
                Layout.fillWidth: true

                text: root.text
                color: AmneziaStyle.color.textInverted
                wrapMode: Text.Wrap
            }

            ParagraphTextType {
                visible: root.actionText !== ""

                text: root.actionText
                color: AmneziaStyle.color.goldenApricot
                font.weight: 500

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                TapHandler {
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    onTapped: {
                        root.close()
                        root.actionTriggered()
                    }
                }
            }
        }
    }

    Timer {
        id: hideTimer

        interval: root.duration
        repeat: false
        onTriggered: root.close()
    }
}
