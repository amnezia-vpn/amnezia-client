import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

import Style 1.0

TextField {
    id: root

    implicitWidth: 358
    implicitHeight: 44

    leftPadding: 52
    rightPadding: clearButton.visible ? 52 : 16
    topPadding: 10
    bottomPadding: 10
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
    EnterKey.type: Qt.EnterKeySearch

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.bgField
        border.width: 1
        border.color: root.activeFocus ? AmneziaStyle.color.borderFocus : AmneziaStyle.color.borderField

        IconImage {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter

            width: 24
            height: 24

            source: "qrc:/images/controls/search.svg"
            sourceSize: Qt.size(24, 24)
            color: AmneziaStyle.color.iconSecondary
        }
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
        anchors.rightMargin: 16 - (width - 24) / 2
        anchors.verticalCenter: parent.verticalCenter

        implicitWidth: Qt.platform.os === "android" ? 48 : 44
        implicitHeight: implicitWidth

        focusPolicy: Qt.NoFocus
        visible: root.text !== "" && !root.readOnly
        icon.source: "qrc:/images/controls/x-circle.svg"
        icon.width: 24
        icon.height: 24
        Accessible.name: qsTr("Clear")
        icon.color: AmneziaStyle.color.iconSecondary

        onClicked: {
            root.clear()
            root.forceActiveFocus()
        }
    }
}
