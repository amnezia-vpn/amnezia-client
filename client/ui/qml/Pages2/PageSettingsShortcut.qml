import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    property bool isMacShortcutSupported: GC.isDesktop() && Qt.platform.os === "osx"

    onVisibleChanged: {
        if (!visible) {
            ShortcutController.cancelRecording()
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if (backButton.enabled && backButton.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Keyboard shortcut")
                descriptionText: qsTr("Configure a global shortcut that toggles the VPN even when the AmneziaVPN window is not focused.")
            }
        }

        model: root.isMacShortcutSupported ? 1 : 0

        delegate: ColumnLayout {
            width: listView.width

            visible: root.isMacShortcutSupported

            SwitcherType {
                Layout.fillWidth: true
                Layout.margins: 16

                enabled: ShortcutController.hasShortcut
                opacity: enabled ? 1.0 : 0.5

                text: qsTr("Enable global shortcut")
                descriptionText: qsTr("Toggles the VPN from anywhere in the system")

                checked: ShortcutController.enabled
                onToggled: function() {
                    if (checked !== ShortcutController.enabled) {
                        ShortcutController.enabled = checked
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Shortcut")
                descriptionText: ShortcutController.shortcutText !== ""
                                 ? ShortcutController.shortcutText
                                 : qsTr("Not configured")
                rightImageSource: ShortcutController.recording ? "" : "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    ShortcutController.startRecording()
                }
            }

            DividerType {}

            Item {
                id: recordingContainer
                visible: ShortcutController.recording
                Layout.fillWidth: true
                implicitHeight: captureCard.implicitHeight + 32

                onVisibleChanged: {
                    if (visible) {
                        focusActivationTimer.restart()
                    }
                }

                Timer {
                    id: focusActivationTimer

                    interval: 0
                    repeat: false
                    onTriggered: captureFocusScope.forceActiveFocus()
                }

                Rectangle {
                    id: captureCard

                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.topMargin: 16

                    implicitHeight: captureContent.implicitHeight + 32
                    radius: 16
                    color: AmneziaStyle.color.onyxBlack
                    border.color: AmneziaStyle.color.paleGray
                    border.width: 1

                    FocusScope {
                        id: captureFocusScope

                        anchors.fill: parent
                        focus: ShortcutController.recording
                        Keys.priority: Keys.BeforeItem

                        Keys.onPressed: function(event) {
                            if (!ShortcutController.recording || event.isAutoRepeat) {
                                return
                            }

                            if (event.key === Qt.Key_Escape) {
                                ShortcutController.cancelRecording()
                                event.accepted = true
                                return
                            }

                            const nativeKeyCode = Number(event.nativeVirtualKey || event.nativeScanCode || 0)
                            ShortcutController.captureShortcut(
                                        event.key,
                                        event.modifiers,
                                        nativeKeyCode)
                            event.accepted = true
                        }

                        ColumnLayout {
                            id: captureContent

                            anchors.fill: parent
                            anchors.margins: 16

                            Header2Type {
                                Layout.fillWidth: true
                                headerText: qsTr("Press the desired shortcut")
                            }

                            ParagraphTextType {
                                Layout.fillWidth: true
                                color: AmneziaStyle.color.mutedGray
                                text: qsTr("Use a combination with Command, Option, Control or Shift. Press Esc to cancel.")
                            }

                            BasicButtonType {
                                Layout.fillWidth: true
                                Layout.topMargin: 8

                                text: qsTr("Cancel")
                                clickedFunc: function() {
                                    ShortcutController.cancelRecording()
                                }
                            }
                        }
                    }
                }
            }

            DividerType {
                visible: ShortcutController.recording
            }

            LabelWithButtonType {
                Layout.fillWidth: true

                visible: ShortcutController.hasShortcut

                text: qsTr("Clear shortcut")
                leftImageSource: "qrc:/images/controls/trash.svg"
                isSmallLeftImage: true

                clickedFunction: function() {
                    ShortcutController.clearShortcut()
                }
            }

            DividerType {
                visible: ShortcutController.hasShortcut
            }
        }

        footer: ColumnLayout {
            width: listView.width
            visible: !root.isMacShortcutSupported

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.margins: 16
                color: AmneziaStyle.color.mutedGray
                text: qsTr("Global shortcut configuration is currently available only for the macOS desktop build.")
            }
        }
    }
}
