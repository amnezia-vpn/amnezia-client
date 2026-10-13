import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

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

    leftPadding: 16
    rightPadding: 8
    topPadding: 12
    bottomPadding: 12

    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose

    onClosed: hideTimer.stop()

    background: Item {
        transform: Translate {
            x: swipe.offsetX
            y: swipe.offsetY
        }

        Rectangle {
            id: surface

            anchors.fill: parent
            radius: 12
            color: AmneziaStyle.color.surfaceInverse
            visible: false
        }

        DropShadow {
            anchors.fill: surface
            source: surface
            verticalOffset: 4
            radius: 8
            samples: 17
            color: Qt.rgba(0, 0, 0, 0.15)
        }

        DropShadow {
            anchors.fill: surface
            source: surface
            verticalOffset: 1
            radius: 3
            samples: 7
            color: Qt.rgba(0, 0, 0, 0.3)
        }
    }

    contentItem: Item {
        id: swipe

        property real offsetX: 0
        property real offsetY: 0

        implicitHeight: content.implicitHeight

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

            spacing: 4

            ParagraphTextType {
                Layout.fillWidth: true

                lineHeight: 18 + LanguageUiController.getLineHeightAppend()
                font.pixelSize: 14

                text: root.text
                color: AmneziaStyle.color.textInverted
                wrapMode: Text.Wrap
            }

            Rectangle {
                id: actionButton

                visible: root.actionText !== ""

                Layout.preferredWidth: actionLabel.implicitWidth + 28
                Layout.preferredHeight: 48

                radius: 16
                color: {
                    if (actionTap.pressed) {
                        return AmneziaStyle.color.surfaceInversePressed
                    }
                    return actionHover.hovered ? AmneziaStyle.color.surfaceInverseHovered
                                               : AmneziaStyle.color.surfaceInverse
                }

                ParagraphTextType {
                    id: actionLabel

                    anchors.centerIn: parent

                    text: root.actionText
                    color: AmneziaStyle.color.goldenApricotLight
                    font.weight: 600
                    font.letterSpacing: -0.4
                    horizontalAlignment: Text.AlignHCenter
                }

                HoverHandler {
                    id: actionHover
                    cursorShape: Qt.PointingHandCursor
                }

                TapHandler {
                    id: actionTap

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
