import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    readonly property int localProxyPortMin: 1024
    readonly property int localProxyPortMax: 65535
    readonly property int defaultLocalProxyPort: 10808

    property string portValidationError: ""
    property int pendingStartRequestedPort: -1
    property int pendingStartAutoSelectedPort: -1
    property bool pendingStartVpnWasActive: false
    property bool pendingEnableAfterVpnDisconnect: false
    property string pendingEnableServerId: ""
    property int pendingEnableRequestedPort: -1
    property int pendingEnableAutoSelectedPort: -1
    property int pendingEnablePortToUse: -1

    function clearPendingEnableAfterVpnDisconnect() {
        root.pendingEnableAfterVpnDisconnect = false
        root.pendingEnableServerId = ""
        root.pendingEnableRequestedPort = -1
        root.pendingEnableAutoSelectedPort = -1
        root.pendingEnablePortToUse = -1
    }

    function enableLocalProxyNow(serverId, requestedPort, autoSelectedPort, portToEnable, vpnWasActive) {
        if (!SettingsController.enableLocalProxy(serverId, portToEnable)) {
            PageController.showNotificationMessage(qsTr("Failed to enable local proxy. Check the port (%1-%2).")
                .arg(root.localProxyPortMin)
                .arg(root.localProxyPortMax))
            return false
        }

        root.pendingStartRequestedPort = requestedPort
        root.pendingStartAutoSelectedPort = autoSelectedPort
        root.pendingStartVpnWasActive = vpnWasActive
        startSuccessToastTimer.restart()
        return true
    }

    function getPortField() {
        var item = listView.itemAtIndex(0)
        return item !== null ? item.children[0] : null
    }

    function computePortErrorText() {
        var portField = getPortField()
        if (portField === null) return ""
        const text = portField.textField.text.trim()
        if (text === "") {
            return qsTr("Enter a port")
        }
        const value = parseInt(text)
        if (isNaN(value) || value < root.localProxyPortMin || value > root.localProxyPortMax) {
            return qsTr("Port must be between %1 and %2")
                .arg(root.localProxyPortMin)
                .arg(root.localProxyPortMax)
        }
        if (SettingsController.isLocalProxyPortBusy(value)) {
            return qsTr("Port %1 is already in use on this device. Choose another one")
                .arg(value)
        }
        return ""
    }

    function handleLocalProxyToggle(checked) {
        if (checked) {
            if (!ServersModel.processedServerIsPremium) {
                PageController.showNotificationMessage(qsTr("Local proxy is available only for Amnezia Premium"))
                return
            }
            const wasVpnActive = ConnectionController.isConnected || ConnectionController.isConnectionInProgress

            let serverId = ServersModel.processedServerId
            if (!serverId) {
                serverId = ServersModel.getServerId(ServersModel.getDefaultServerIndex())
            }
            if (!serverId) {
                PageController.showNotificationMessage(qsTr("Unable to determine the current server"))
                return
            }

            if (SettingsController.isLocalProxyHttpEnabled
                    && SettingsController.localProxyOwnerId
                    && SettingsController.localProxyOwnerId !== serverId) {
                PageController.showNotificationMessage(qsTr("Local proxy is already enabled for another server"))
                return
            }

            const requestedPort = SettingsController.localProxyPort
            if (requestedPort < root.localProxyPortMin || requestedPort > root.localProxyPortMax) {
                PageController.showNotificationMessage(qsTr("Port must be between %1 and %2")
                    .arg(root.localProxyPortMin)
                    .arg(root.localProxyPortMax))
                return
            }

            let autoSelectedPort = -1
            if (SettingsController.isLocalProxyPortBusy(requestedPort)) {
                if (SettingsController.isLocalProxyPortUserDefined()
                        || requestedPort !== root.defaultLocalProxyPort) {
                    PageController.showNotificationMessage(qsTr("Port %1 is already in use on this device. Choose another one")
                        .arg(requestedPort))
                    return
                }

                autoSelectedPort = SettingsController.findFirstAvailableLocalProxyPort(root.defaultLocalProxyPort + 1)
                if (autoSelectedPort <= 0) {
                    PageController.showNotificationMessage(qsTr("Port %1 is already in use on this device. Choose another one")
                        .arg(requestedPort))
                    return
                }
            }

            const portToEnable = autoSelectedPort > 0 ? autoSelectedPort : requestedPort
            if (wasVpnActive) {
                root.pendingEnableAfterVpnDisconnect = true
                root.pendingEnableServerId = serverId
                root.pendingEnableRequestedPort = requestedPort
                root.pendingEnableAutoSelectedPort = autoSelectedPort
                root.pendingEnablePortToUse = portToEnable
                ConnectionController.closeConnection()
                return
            }

            root.enableLocalProxyNow(serverId, requestedPort, autoSelectedPort, portToEnable, false)
        } else {
            startSuccessToastTimer.stop()
            root.clearPendingEnableAfterVpnDisconnect()
            root.pendingStartRequestedPort = -1
            root.pendingStartAutoSelectedPort = -1
            root.pendingStartVpnWasActive = false
            SettingsController.disableLocalProxy()
            PageController.showNotificationMessage(qsTr("Local proxy stopped"))
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

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

            HeaderTypeWithSwitcher {
                id: localProxyHeader

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("Local Proxy")
                descriptionText: qsTr("Use a proxy to route selected apps (for example, the CensorTracker extension) through Amnezia Premium.")
                showSwitcher: ServersModel.processedServerIsPremium
                switcher {
                    checked: SettingsController.isLocalProxyHttpEnabled
                }
                switcherFunction: function(checked) {
                    // Ignore UI sync toggles; react only to real state change intent.
                    if (checked === SettingsController.isLocalProxyHttpEnabled) {
                        return
                    }
                    root.handleLocalProxyToggle(checked)
                    // Keep checked declaratively linked after any user interaction path.
                    localProxyHeader.switcher.checked = Qt.binding(function() {
                        return SettingsController.isLocalProxyHttpEnabled
                    })
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                color: localProxyHeader.descriptionColor
                text: qsTr("Only one can be on at a time: VPN or local proxy.")
            }

            BasicButtonType {
                Layout.topMargin: 8
                Layout.leftMargin: 8
                Layout.bottomMargin: 28
                implicitHeight: 32

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Learn more")
                clickedFunc: function() {
                    const path = LanguageModel.currentLanguageName === "Русский"
                        ? "ru/documentation/instructions/local-proxy"
                        : "documentation/instructions/local-proxy"
                    Qt.openUrlExternally(LanguageModel.getCurrentDocsUrl(path))
                }
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout {
            width: listView.width
            spacing: 16

            TextFieldWithHeaderType {
                id: portField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Address and port")
                buttonText: qsTr("Copy")
                errorText: root.portValidationError
                clearErrorOnTextChanged: false

                enabled: true
                rightButtonClickedOnEnter: false

                clickedFunc: function() {
                    const portText = portField.effectivePortText()
                    GC.copyToClipBoard("127.0.0.1:" + portText)
                    PageController.showNotificationMessage(qsTr("Copied: 127.0.0.1:%1").arg(portText))
                }

                textField.validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,5}$/
                }
                textField.leftPadding: portPrefix.implicitWidth
                textField.placeholderText: root.defaultLocalProxyPort.toString()
                textField.inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText

                function syncPortValue() {
                    const port = SettingsController.localProxyPort
                    const isValidPort = port >= root.localProxyPortMin && port <= root.localProxyPortMax
                    textField.text = isValidPort ? port.toString() : ""
                }

                function portValue() {
                    const value = parseInt(textField.text)
                    return isNaN(value) ? -1 : value
                }

                function effectivePortText() {
                    const value = portValue()
                    if (value >= root.localProxyPortMin && value <= root.localProxyPortMax) {
                        return value.toString()
                    }
                    const fallback = SettingsController.localProxyPort
                    if (fallback >= root.localProxyPortMin && fallback <= root.localProxyPortMax) {
                        return fallback.toString()
                    }
                    return root.defaultLocalProxyPort.toString()
                }

                Component.onCompleted: syncPortValue()

                textField.onTextChanged: {
                    if (textField.activeFocus) {
                        root.portValidationError = ""
                    }
                }
            }

            Text {
                id: portPrefix

                parent: portField.textField
                text: "127.0.0.1:"
                color: AmneziaStyle.color.paleGray
                font.pixelSize: portField.textField.font.pixelSize
                font.weight: portField.textField.font.weight
                font.family: portField.textField.font.family
                z: 1

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Save")
                enabled: true

                clickedFunc: function() {
                    if (SettingsController.isLocalProxyHttpEnabled) {
                        PageController.showNotificationMessage(qsTr("Disable Local Proxy to change the port"))
                        return
                    }
                    const validationError = root.computePortErrorText()
                    root.portValidationError = validationError
                    if (validationError !== "") {
                        return
                    }

                    const value = portField.portValue()
                    if (!SettingsController.setLocalProxyPort(value)) {
                        PageController.showNotificationMessage(qsTr("Failed to save port. Valid range: %1-%2")
                            .arg(root.localProxyPortMin)
                            .arg(root.localProxyPortMax))
                    } else {
                        PageController.showNotificationMessage(qsTr("Port saved: %1").arg(value))
                    }
                    portField.syncPortValue()
                }
            }
        }
    }

    Timer {
        id: startSuccessToastTimer
        interval: 250
        repeat: false
        running: false
        onTriggered: {
            if (!SettingsController.isLocalProxyHttpEnabled) {
                return
            }

            if (root.pendingStartAutoSelectedPort > 0) {
                PageController.showNotificationMessage(qsTr("Port %1 is in use — selected free port %2.")
                    .arg(root.defaultLocalProxyPort)
                    .arg(root.pendingStartAutoSelectedPort))
            } else if (root.pendingStartVpnWasActive && root.pendingStartRequestedPort > 0) {
                PageController.showNotificationMessage(qsTr("VPN turned off. Local proxy is running: 127.0.0.1:%1")
                    .arg(root.pendingStartRequestedPort))
            } else if (root.pendingStartRequestedPort > 0) {
                PageController.showNotificationMessage(qsTr("Local proxy is running: 127.0.0.1:%1")
                    .arg(root.pendingStartRequestedPort))
            }

            root.pendingStartRequestedPort = -1
            root.pendingStartAutoSelectedPort = -1
            root.pendingStartVpnWasActive = false
        }
    }

    Connections {
        target: ConnectionController

        function onConnectionStateChanged() {
            if (!root.pendingEnableAfterVpnDisconnect) {
                return
            }

            if (ConnectionController.isConnected || ConnectionController.isConnectionInProgress) {
                return
            }

            const serverId = root.pendingEnableServerId
            const requestedPort = root.pendingEnableRequestedPort
            const autoSelectedPort = root.pendingEnableAutoSelectedPort
            const portToEnable = root.pendingEnablePortToUse
            root.clearPendingEnableAfterVpnDisconnect()

            root.enableLocalProxyNow(serverId, requestedPort, autoSelectedPort, portToEnable, true)
        }
    }

    Connections {
        target: SettingsController

        function onLocalProxySettingsUpdated() {
            var portField = root.getPortField()
            if (portField !== null && !portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }

        function onLocalProxyStartFailed(message) {
            startSuccessToastTimer.stop()
            root.pendingStartRequestedPort = -1
            root.pendingStartAutoSelectedPort = -1
            root.pendingStartVpnWasActive = false
            PageController.showNotificationMessage(message)
        }
    }

    Connections {
        target: ServersModel

        function onProcessedServerChanged() {
            var portField = root.getPortField()
            if (portField !== null && !portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }
    }
}
