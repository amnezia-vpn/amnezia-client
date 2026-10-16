import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import Style 1.0

import "TextTypes"

Button {
    id: root

    property bool joined: false
    property string imageSource

    implicitWidth: root.imageSource !== "" ? 48 : Math.max(83, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: 56

    horizontalPadding: 12
    verticalPadding: 8

    hoverEnabled: true

    opacity: root.enabled ? 1 : 0.4

    background: Rectangle {
        radius: 16
        topLeftRadius: root.joined ? 0 : 16
        bottomLeftRadius: root.joined ? 0 : 16

        color: {
            if (root.down) {
                return AmneziaStyle.color.bgActionPress
            }
            return root.hovered ? AmneziaStyle.color.bgActionHover : AmneziaStyle.color.bgAction
        }

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        FocusRingType {
            radius: 16
            visible: root.visualFocus
        }
    }

    contentItem: Item {
        implicitWidth: root.imageSource !== "" ? 24 : label.implicitWidth
        implicitHeight: 24

        IconImage {
            anchors.centerIn: parent

            width: 24
            height: 24

            visible: root.imageSource !== ""
            source: root.imageSource
            sourceSize: Qt.size(24, 24)
            color: AmneziaStyle.color.iconInverted
        }

        ButtonTextType {
            id: label

            anchors.centerIn: parent
            width: Math.min(implicitWidth, parent.width)

            visible: root.imageSource === "" && text !== ""
            text: root.text

            color: AmneziaStyle.color.textInverted
            lineHeight: 18 + LanguageUiController.getLineHeightAppend()
            font.pixelSize: 14

            wrapMode: Text.NoWrap
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
