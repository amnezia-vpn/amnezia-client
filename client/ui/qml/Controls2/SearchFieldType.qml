import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import Style 1.0

TextField {
    id: root

    implicitWidth: 358
    implicitHeight: 64

    leftPadding: 16
    rightPadding: clearButton.visible ? 56 : 16
    topPadding: 20
    bottomPadding: 20
    verticalAlignment: TextInput.AlignVCenter

    opacity: root.enabled ? 1 : 0.4

    color: AmneziaStyle.color.textPrimary
    placeholderText: qsTr("Search")
    placeholderTextColor: AmneziaStyle.color.textTertiary
    selectionColor: AmneziaStyle.color.accentDeep
    selectedTextColor: AmneziaStyle.color.textPrimary

    font.pixelSize: 16
    font.weight: 400
    font.family: "PT Root UI VF"
    font.letterSpacing: -0.4
    font.features: { "ss08": 1, "ss09": 1, "lnum": 1, "tnum": 1 }

    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.bgSurfaceAlt
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? AmneziaStyle.color.borderEmphasis : AmneziaStyle.color.borderSoft
    }

    cursorDelegate: Rectangle {
        id: caret

        width: 2
        height: 22
        radius: 1
        color: AmneziaStyle.color.accentLight
        visible: root.activeFocus && !root.readOnly && root.selectionStart === root.selectionEnd

        Timer {
            interval: Qt.styleHints.cursorFlashTime / 2
            running: caret.visible && Qt.styleHints.cursorFlashTime > 0
            repeat: true
            onTriggered: caret.opacity = caret.opacity > 0 ? 0 : 1
            onRunningChanged: caret.opacity = 1
        }

        Connections {
            target: root
            function onCursorPositionChanged() {
                caret.opacity = 1
            }
        }
    }

    Keys.onEscapePressed: function(event) {
        if (root.text === "") {
            event.accepted = false
            return
        }
        root.clear()
    }

    ContextMenu.menu: contextMenu.useNativeEditMenu ? null : contextMenu
    ContextMenu.onRequested: function(position) {
        contextMenu.requestNative(position)
    }

    TapHandler {
        enabled: contextMenu.useNativeEditMenu
        acceptedDevices: PointerDevice.TouchScreen
        onDoubleTapped: function(eventPoint) {
            root.forceActiveFocus()
            root.cursorPosition = root.positionAt(eventPoint.position.x, eventPoint.position.y)
            root.selectWord()
            contextMenu.requestNative(eventPoint.position)
        }
    }

    ContextMenuType {
        id: contextMenu
        textObj: root
    }

    IconButtonType {
        id: clearButton

        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        implicitWidth: 40
        implicitHeight: 40

        visible: root.text !== "" && !root.readOnly
        icon.source: "qrc:/images/controls/x-circle.svg"
        icon.color: AmneziaStyle.color.iconSecondary

        onClicked: {
            root.clear()
            root.forceActiveFocus()
        }
    }
}
