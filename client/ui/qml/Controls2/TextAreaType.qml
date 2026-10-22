import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Rectangle {
    id: root

    property alias textArea: textArea
    property alias textAreaText: textArea.text
    property alias placeholderText: textArea.placeholderText

    property int maximumLength: 0
    property bool clearable: true

    readonly property bool atLimit: root.maximumLength > 0 && textArea.length >= root.maximumLength

    implicitWidth: 358
    implicitHeight: 180

    radius: 14
    color: AmneziaStyle.color.bgSurfaceAlt
    border.width: textArea.activeFocus || root.atLimit || textArea.length === 0 ? 1 : 0
    border.color: {
        if (root.atLimit) {
            return AmneziaStyle.color.statusDestructive
        }
        return textArea.activeFocus ? AmneziaStyle.color.borderEmphasis : AmneziaStyle.color.borderSoft
    }

    opacity: root.enabled ? 1 : 0.4

    Flickable {
        id: flickable

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        anchors.topMargin: 1
        anchors.leftMargin: 1
        anchors.rightMargin: 1

        clip: true
        boundsBehavior: Flickable.StopAtBounds

        TextArea.flickable: TextArea {
            id: textArea

            leftPadding: 11
            rightPadding: 11
            topPadding: 15
            bottomPadding: 8

            color: AmneziaStyle.color.textPrimary
            placeholderTextColor: AmneziaStyle.color.textTertiary
            selectionColor: AmneziaStyle.color.accentDeep
            selectedTextColor: AmneziaStyle.color.textPrimary

            font.pixelSize: 16
            font.weight: 400
            font.family: "PT Root UI VF"
            font.letterSpacing: -0.4
            font.features: { "ss08": 1, "ss09": 1, "lnum": 1, "tnum": 1 }

            wrapMode: Text.Wrap
            background: null

            onTextChanged: {
                if (root.maximumLength > 0 && length > root.maximumLength) {
                    const position = Math.min(cursorPosition, root.maximumLength)
                    text = text.substring(0, root.maximumLength)
                    cursorPosition = position
                }
            }

            ContextMenu.onRequested: function(position) {
                contextMenu.request(position)
            }

            TapHandler {
                enabled: contextMenu.useNativeEditMenu
                acceptedDevices: PointerDevice.TouchScreen
                onDoubleTapped: function(eventPoint) {
                    textArea.forceActiveFocus()
                    textArea.cursorPosition = textArea.positionAt(eventPoint.position.x, eventPoint.position.y)
                    textArea.selectWord()
                    contextMenu.requestNative(eventPoint.position)
                }
            }

            ContextMenuType {
                id: contextMenu
                textObj: textArea
            }
        }
    }

    RowLayout {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.bottomMargin: 16

        height: 40
        spacing: 12

        SmallTextType {
            Layout.fillWidth: true

            visible: root.maximumLength > 0
            text: textArea.length + "/" + root.maximumLength
            color: root.atLimit ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.textQuaternary
        }

        Item {
            Layout.fillWidth: true
            visible: root.maximumLength <= 0
        }

        IconButtonType {
            implicitWidth: 40
            implicitHeight: 40

            visible: root.clearable && textArea.length > 0 && !textArea.readOnly
            icon.source: "qrc:/images/controls/trash.svg"

            onClicked: {
                textArea.clear()
                textArea.forceActiveFocus()
            }
        }
    }
}
