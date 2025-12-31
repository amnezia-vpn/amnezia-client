import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"

PageType {
    id: root

    readonly property int localProxyPortMin: 1024
    readonly property int localProxyPortMax: 65535

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onActiveFocusChanged: {
            if (activeFocus) {
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
                headerText: qsTr("Local proxy")
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout {
            width: listView.width
            spacing: 16

            SwitcherType {
                id: localProxySwitch

                Layout.fillWidth: true
                Layout.margins: 16

                property string statusText: ""

                text: qsTr("Enable local proxy")
                descriptionText: statusText

                checked: SettingsController.isLocalProxyHttpEnabled

                function computeStatusText() {
                    statusText = SettingsController.isLocalProxyHttpEnabled
                                 ? qsTr("Running: 127.0.0.1:%1").arg(SettingsController.localProxyPort || 0)
                                 : qsTr("Disabled")
                }

                function syncState() {
                    computeStatusText()
                    if (checked !== SettingsController.isLocalProxyHttpEnabled) {
                        checked = SettingsController.isLocalProxyHttpEnabled
                    }
                }

                Component.onCompleted: syncState()

                onToggled: function() {
                    if (checked) {
                        const serverUuid = ServersModel.processedServerUuid
                        if (!serverUuid) {
                            checked = false
                            PageController.showNotificationMessage(qsTr("Unable to determine the current server"))
                            return
                        }

                        if (SettingsController.isLocalProxyHttpEnabled
                                && SettingsController.localProxyOwnerUuid
                                && SettingsController.localProxyOwnerUuid !== serverUuid) {
                            checked = false
                            PageController.showNotificationMessage(qsTr("Local proxy is already enabled for another server"))
                            return
                        }

                        const requestedPort = portField.portValue()
                        if (requestedPort < root.localProxyPortMin || requestedPort > root.localProxyPortMax) {
                            checked = false
                            PageController.showNotificationMessage(qsTr("Port must be between %1 and %2")
                                                                   .arg(root.localProxyPortMin)
                                                                   .arg(root.localProxyPortMax))
                            return
                        }

                        if (!SettingsController.enableLocalProxy(serverUuid, requestedPort)) {
                            checked = false
                            PageController.showNotificationMessage(qsTr("Failed to enable local proxy. Check the port (%1-%2).")
                                                                   .arg(root.localProxyPortMin)
                                                                   .arg(root.localProxyPortMax))
                            localProxySwitch.syncState()
                            return
                        }
                        localProxySwitch.syncState()
                    } else {
                        SettingsController.disableLocalProxy()
                        localProxySwitch.syncState()
                    }
                }
            }

            DividerType {}

            TextFieldWithHeaderType {
                id: portField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("HTTP API port")

                enabled: true

                textField.validator: IntValidator {
                    bottom: root.localProxyPortMin
                    top: root.localProxyPortMax
                }

                function syncPortValue() {
                    const port = SettingsController.localProxyPort
                    textField.text = (port >= root.localProxyPortMin && port <= root.localProxyPortMax) ? port.toString() : ""
                }

                function portValue() {
                    const value = parseInt(textField.text)
                    return isNaN(value) ? -1 : value
                }

                Component.onCompleted: syncPortValue()

                textField.onEditingFinished: {
                    const value = portField.portValue()
                    if (value < root.localProxyPortMin || value > root.localProxyPortMax) {
                        PageController.showNotificationMessage(qsTr("Port must be between %1 and %2")
                                                               .arg(root.localProxyPortMin)
                                                               .arg(root.localProxyPortMax))
                        portField.syncPortValue()
                        return
                    }

                    if (!SettingsController.setLocalProxyPort(value)) {
                        PageController.showNotificationMessage(qsTr("Failed to save port. Valid range: %1-%2")
                                                               .arg(root.localProxyPortMin)
                                                               .arg(root.localProxyPortMax))

                    }
                    
                    portField.syncPortValue()
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("HTTP API controls Xray via /api/v1/up and /api/v1/down. SOCKS inbound stays on port 10808.")
            }
        }
    }

    Connections {
        target: SettingsController

        function onLocalProxySettingsUpdated() {
            localProxySwitch.syncState()
            if (!portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }
    }

    Connections {
        target: ServersModel

        function onProcessedServerChanged() {
            localProxySwitch.syncState()
            if (!portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }
    }
}

