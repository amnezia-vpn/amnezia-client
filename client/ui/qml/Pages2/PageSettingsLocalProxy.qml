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

    readonly property bool isEnabledForThisServer: SettingsController.isLocalProxyHttpEnabled
                                                   && SettingsController.localProxyOwnerId === ServersUiController.processedServerId
    readonly property bool isOwnedByOtherServer: SettingsController.isLocalProxyHttpEnabled
                                                 && SettingsController.localProxyOwnerId !== ""
                                                 && SettingsController.localProxyOwnerId !== ServersUiController.processedServerId

    property int pendingStartRequestedPort: -1
    property bool pendingStartVpnWasActive: false
    property bool pendingEnableAfterVpnDisconnect: false
    property string pendingEnableServerId: ""
    property int pendingEnableRequestedPort: -1

    function clearPendingEnableAfterVpnDisconnect() {
        root.pendingEnableAfterVpnDisconnect = false
        root.pendingEnableServerId = ""
        root.pendingEnableRequestedPort = -1
    }

    function enableLocalProxyNow(serverId, requestedPort, vpnWasActive) {
        if (!SettingsController.enableLocalProxy(serverId, requestedPort)) {
            PageController.showNotificationMessage(qsTr("Failed to enable local proxy. Check the port (%1-%2).")
                .arg(root.localProxyPortMin)
                .arg(root.localProxyPortMax))
            return false
        }

        root.pendingStartRequestedPort = requestedPort
        root.pendingStartVpnWasActive = vpnWasActive
        startSuccessToastTimer.restart()
        return true
    }

    function computePortErrorText() {
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
            if (!ServersUiController.processedServerIsPremium) {
                PageController.showNotificationMessage(qsTr("Local proxy is available only for Amnezia Premium"))
                return
            }
            const wasVpnActive = ConnectionController.isConnected || ConnectionController.isConnectionInProgress

            let serverId = ServersUiController.processedServerId
            if (!serverId) {
                serverId = ServersUiController.defaultServerId
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

            if (SettingsController.isLocalProxyPortBusy(requestedPort)
                    && SettingsController.isLocalProxyPortUserDefined()) {
                PageController.showNotificationMessage(qsTr("Port %1 is already in use on this device. Choose another one")
                    .arg(requestedPort))
                return
            }

            if (wasVpnActive) {
                root.pendingEnableAfterVpnDisconnect = true
                root.pendingEnableServerId = serverId
                root.pendingEnableRequestedPort = requestedPort
                ConnectionController.closeConnection()
                return
            }

            root.enableLocalProxyNow(serverId, requestedPort, false)
        } else {
            startSuccessToastTimer.stop()
            root.clearPendingEnableAfterVpnDisconnect()
            root.pendingStartRequestedPort = -1
            root.pendingStartVpnWasActive = false
            SettingsController.disableLocalProxy()
            PageController.showNotificationMessage(qsTr("Local proxy stopped"))
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                id: navigationBar

                Layout.fillWidth: true

                title: qsTr("Local Proxy")
                description: qsTr("Use a proxy to route selected apps (for example, the CensorTracker extension) through Amnezia Premium.")

                header.showSwitcher: ServersUiController.processedServerIsPremium
                header.switcher.checked: root.isEnabledForThisServer
                header.switcher.enabled: !root.isOwnedByOtherServer

                Connections {
                    target: navigationBar.header

                    function onSwitcherToggled(checked) {
                        if (checked !== root.isEnabledForThisServer) {
                            root.handleLocalProxyToggle(checked)
                        }
                        navigationBar.header.switcher.checked = Qt.binding(function() {
                            return root.isEnabledForThisServer
                        })
                    }
                }
            }

            NotificationType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: root.isOwnedByOtherServer

                variant: NotificationType.Warning
                text: qsTr("Local proxy is already running for \"%1\". Turn it off there to use it with this server.")
                    .arg(ServersUiController.serverName(SettingsController.localProxyOwnerId))
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Only one can be on at a time: VPN or local proxy.")
            }

            LinkButtonType {
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                size: LinkButtonType.Medium
                text: qsTr("Learn more")

                onClicked: {
                    const path = LanguageUiController.currentLanguageName === "Русский"
                        ? "ru/documentation/instructions/local-proxy"
                        : "documentation/instructions/local-proxy"
                    Qt.openUrlExternally(LanguageUiController.getCurrentDocsUrl(path))
                }
            }

            TextFieldType {
                id: portField

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                headerText: qsTr("Address and port")
                buttonText: qsTr("Copy")
                placeholderText: root.defaultLocalProxyPort.toString()

                textField.leftPadding: 16 + portPrefix.implicitWidth
                textField.inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                textField.validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,5}$/
                }

                clickedFunc: function() {
                    const portText = portField.effectivePortText()
                    GC.copyToClipBoard("127.0.0.1:" + portText)
                    PageController.showNotificationMessage(qsTr("Copied: 127.0.0.1:%1").arg(portText))
                }

                function syncPortValue() {
                    let port = SettingsController.localProxyPort
                    if (root.isEnabledForThisServer && SettingsController.localProxyActivePort > 0) {
                        port = SettingsController.localProxyActivePort
                    }
                    const isValidPort = port >= root.localProxyPortMin && port <= root.localProxyPortMax
                    textField.text = isValidPort ? port.toString() : ""
                }

                function portValue() {
                    const value = parseInt(textField.text)
                    return isNaN(value) ? -1 : value
                }

                function effectivePortText() {
                    if (root.isEnabledForThisServer && SettingsController.localProxyActivePort > 0) {
                        return SettingsController.localProxyActivePort.toString()
                    }
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

                Text {
                    id: portPrefix

                    parent: portField.textField

                    x: 16
                    y: portField.textField.topPadding
                       + (portField.textField.height - portField.textField.topPadding - portField.textField.bottomPadding - height) / 2
                    z: 1

                    text: "127.0.0.1:"
                    color: AmneziaStyle.color.textSecondary
                    font: portField.textField.font
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ButtonType {
            Layout.fillWidth: true

            text: qsTr("Save")

            onClicked: {
                if (SettingsController.isLocalProxyHttpEnabled) {
                    PageController.showNotificationMessage(qsTr("Disable Local Proxy to change the port"))
                    return
                }
                const validationError = root.computePortErrorText()
                portField.errorText = validationError
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

    Timer {
        id: startSuccessToastTimer
        interval: 250
        repeat: false
        running: false
        onTriggered: {
            if (!SettingsController.isLocalProxyHttpEnabled) {
                return
            }

            const activePort = SettingsController.localProxyActivePort
            const requestedPort = root.pendingStartRequestedPort
            const shownPort = activePort > 0 ? activePort : requestedPort

            if (activePort > 0 && requestedPort > 0 && activePort !== requestedPort) {
                PageController.showNotificationMessage(qsTr("Port %1 is in use — selected free port %2.")
                    .arg(requestedPort)
                    .arg(activePort))
            } else if (root.pendingStartVpnWasActive && shownPort > 0) {
                PageController.showNotificationMessage(qsTr("VPN turned off. Local proxy is running: 127.0.0.1:%1")
                    .arg(shownPort))
            } else if (shownPort > 0) {
                PageController.showNotificationMessage(qsTr("Local proxy is running: 127.0.0.1:%1")
                    .arg(shownPort))
            }

            root.pendingStartRequestedPort = -1
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
            root.clearPendingEnableAfterVpnDisconnect()

            root.enableLocalProxyNow(serverId, requestedPort, true)
        }
    }

    Connections {
        target: SettingsController

        function onLocalProxySettingsUpdated() {
            if (!portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }

        function onLocalProxyStartFailed(message) {
            startSuccessToastTimer.stop()
            root.pendingStartRequestedPort = -1
            root.pendingStartVpnWasActive = false
            PageController.showNotificationMessage(message)
        }
    }

    Connections {
        target: ServersUiController

        function onProcessedServerIdChanged() {
            if (!portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }
    }
}
