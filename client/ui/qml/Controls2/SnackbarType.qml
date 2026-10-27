import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import Style 1.0

import "TextTypes"

Popup {
    id: root

    property string text
    property string actionText
    property bool closable: false
    property int duration: 4000
    property real bottomOffset: PageController.safeAreaBottomMargin

    signal actionTriggered()

    function show(message, action) {
        root.text = message
        root.actionText = action || ""
        swipe.offsetX = 0
        swipe.offsetY = 0
        root.open()
        if (root.duration > 0) {
            hideTimer.restart()
        }
    }

    parent: Overlay.overlay

    z: 1

    width: Math.min(parent.width - 32, 520)
    x: Math.round((parent.width - width) / 2)
    y: parent.height - height - 16 - root.bottomOffset - PageController.imeHeight

    leftPadding: 16
    rightPadding: actionButton.visible ? 8 : 16
    topPadding: 12
    bottomPadding: 12

    modal: false
    focus: root.closable
    closePolicy: root.closable ? Popup.CloseOnEscape : Popup.NoAutoClose

    onOpened: {
        if (root.closable) {
            actionButton.forceActiveFocus()
        }
    }

    onClosed: hideTimer.stop()

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
    }

    background: Item {
        transform: Translate {
            x: swipe.offsetX
            y: swipe.offsetY
        }

        Rectangle {
            id: shadowSource

            anchors.fill: parent

            radius: 12
            color: AmneziaStyle.color.bgSurfaceInverse
            visible: false
        }

        Rectangle {
            id: spreadShadowSource

            x: -3
            y: -3 + 4
            width: parent.width + 6
            height: parent.height + 6

            radius: 15
            color: AmneziaStyle.color.shadowLayer1
            visible: false
        }

        MultiEffect {
            anchors.fill: spreadShadowSource

            source: spreadShadowSource
            blurEnabled: true
            blurMax: 48
            blur: 0.5
        }

        ShadowType {
            anchors.fill: shadowSource
            source: shadowSource
            verticalOffset: 1
            blur: 3
            color: AmneziaStyle.color.shadowLayer2
        }

        Rectangle {
            id: surface

            anchors.fill: parent

            radius: 12
            color: AmneziaStyle.color.bgSurfaceInverse
        }
    }

    contentItem: Item {
        id: swipe

        property real offsetX: 0
        property real offsetY: 0

        implicitHeight: Math.max(content.implicitHeight, actionButton.visible ? 40 : 0)

        transform: Translate {
            x: swipe.offsetX
            y: swipe.offsetY
        }

        TapHandler {
            enabled: !root.closable
            onTapped: root.close()
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
                if (root.duration > 0) {
                    hideTimer.restart()
                }
            }
        }

        RowLayout {
            id: content

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            spacing: 4

            SmallTextType {
                Layout.fillWidth: true

                text: root.text.replace(/\n/g, "<br>").replace(/<a /g, "<a style=\"color: " + AmneziaStyle.color.textInfoInverse + "; text-decoration: none\" ")
                textFormat: Text.RichText
                color: AmneziaStyle.color.textInverted

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(LanguageUiController.getCurrentDocsUrl(link))
                }

                HoverHandler {
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            AbstractButton {
                id: actionButton

                Layout.alignment: Qt.AlignVCenter
                Layout.maximumWidth: content.width / 2

                visible: root.actionText !== "" || root.closable

                implicitWidth: actionLabel.implicitWidth + 28
                implicitHeight: 40

                hoverEnabled: true

                text: root.actionText !== "" ? root.actionText : qsTr("Close")

                background: Rectangle {
                    radius: 16
                    color: {
                        if (actionButton.down) {
                            return AmneziaStyle.color.bgActionPress
                        }
                        return actionButton.hovered ? AmneziaStyle.color.bgSurfaceInverseAlt
                                                    : AmneziaStyle.color.bgSurfaceInverse
                    }

                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }

                    FocusRingType {
                        radius: 16
                        visible: actionButton.visualFocus
                    }
                }

                contentItem: BodyTextType {
                    id: actionLabel

                    text: actionButton.text
                    color: AmneziaStyle.color.textInverted
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.NoWrap
                    elide: Text.ElideRight
                }

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                onClicked: {
                    const hasAction = root.actionText !== ""
                    root.close()
                    if (hasAction) {
                        root.actionTriggered()
                    }
                }

                Keys.onEnterPressed: actionButton.clicked()
                Keys.onReturnPressed: actionButton.clicked()
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
