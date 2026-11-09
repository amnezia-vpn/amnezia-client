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

    readonly property bool isEnabledForThisServer: LocalProxyController.isEnabled
                                                   && LocalProxyController.ownerId === ServersUiController.processedServerId
    readonly property bool isOwnedByOtherServer: LocalProxyController.isEnabled
                                                 && LocalProxyController.ownerId !== ""
                                                 && LocalProxyController.ownerId !== ServersUiController.processedServerId

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
        if (!LocalProxyController.enable(serverId, requestedPort)) {
            PageController.showNotificationMessage(qsTr("Failed to enable local proxy. Check the port (%1-%2)")
                .arg(root.localProxyPortMin)
                .arg(root.localProxyPortMax))
            return false
        }

        root.pendingStartRequestedPort = requestedPort
        root.pendingStartVpnWasActive = vpnWasActive
        startSuccessToastTimer.restart()
        return true
    }

    function portRangeErrorText() {
        return qsTr("Port must be between %1 and %2").arg(root.localProxyPortMin).arg(root.localProxyPortMax)
    }

    function portBusyErrorText(port) {
        return qsTr("Port %1 is already in use on this device. Choose a different one").arg(port)
    }

    function runningPortText() {
        const activePort = LocalProxyController.activePort
        return (activePort > 0 ? activePort : LocalProxyController.port).toString()
    }

    function computePortErrorText() {
        const text = portField.textField.text.trim()
        if (text === "") {
            return qsTr("Port is required")
        }
        const value = parseInt(text)
        if (isNaN(value) || value < root.localProxyPortMin || value > root.localProxyPortMax) {
            return root.portRangeErrorText()
        }
        const isOwnActivePort = root.isEnabledForThisServer && value === LocalProxyController.activePort
        if (!isOwnActivePort && LocalProxyController.isPortBusy(value)) {
            return root.portBusyErrorText(value)
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

            if (LocalProxyController.isEnabled
                    && LocalProxyController.ownerId
                    && LocalProxyController.ownerId !== serverId) {
                PageController.showNotificationMessage(qsTr("Local proxy is already enabled for another server"))
                return
            }

            let requestedPort = LocalProxyController.port
            if (portField.textField.text.trim() !== "") {
                const validationError = root.computePortErrorText()
                if (validationError !== "") {
                    portField.errorText = validationError
                    return
                }
                requestedPort = portField.portValue()
                LocalProxyController.setPort(requestedPort)
            } else if (requestedPort < root.localProxyPortMin || requestedPort > root.localProxyPortMax) {
                portField.errorText = root.portRangeErrorText()
                return
            } else if (LocalProxyController.isPortBusy(requestedPort)
                       && LocalProxyController.isPortUserDefined()) {
                portField.errorText = root.portBusyErrorText(requestedPort)
                return
            }

            portField.errorText = ""

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
            LocalProxyController.disable()
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

                title: qsTr("Local proxy")
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 8

                ListItemType {
                    Layout.fillWidth: true

                    visible: GC.isDesktop() && ServersUiController.processedServerIsPremium
                    enabled: !root.isOwnedByOtherServer

                    text: qsTr("Local proxy")

                    onClicked: {
                        localProxySwitch.toggle()
                        localProxySwitch.toggled()
                    }

                    SwitchType {
                        id: localProxySwitch

                        focusPolicy: Qt.NoFocus

                        checked: root.isEnabledForThisServer

                        onToggled: {
                            if (checked !== root.isEnabledForThisServer) {
                                root.handleLocalProxyToggle(checked)
                            }
                            localProxySwitch.checked = Qt.binding(function() {
                                return root.isEnabledForThisServer
                            })
                        }
                    }
                }

                BodyTextType {
                    Layout.fillWidth: true

                    text: qsTr("Used by specific apps (e.g., CensorTracker extension) via Amnezia Premium.\nOnly one can be active at a time: VPN or local proxy")
                    color: AmneziaStyle.color.textSecondary
                }
            }

            NotificationType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.isOwnedByOtherServer

                variant: NotificationType.Warning
                text: qsTr("Local proxy is already running for \"%1\". Turn it off there to use it with this server")
                    .arg(ServersUiController.serverName(LocalProxyController.ownerId))
            }

            ButtonType {
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                variant: ButtonType.Text
                size: ButtonType.Medium
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
                Layout.bottomMargin: 24

                headerText: qsTr("Address and port")
                placeholderText: LocalProxyController.port.toString()
                trailingIconSource: root.isEnabledForThisServer ? "qrc:/images/controls/copy.svg" : ""

                textField.leftPadding: 16 + portPrefix.implicitWidth
                textField.inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
                textField.validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,5}$/
                }

                onTrailingClicked: {
                    const portText = root.runningPortText()
                    GC.copyToClipBoard("127.0.0.1:" + portText)
                    PageController.showNotificationMessage(qsTr("Copied: 127.0.0.1:%1").arg(portText))
                }

                function syncPortValue() {
                    if (root.isEnabledForThisServer && LocalProxyController.activePort > 0) {
                        textField.text = LocalProxyController.activePort.toString()
                    } else if (LocalProxyController.isPortUserDefined()) {
                        textField.text = LocalProxyController.port.toString()
                    } else {
                        textField.text = ""
                    }
                }

                function portValue() {
                    const value = parseInt(textField.text)
                    return isNaN(value) ? -1 : value
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
                    color: AmneziaStyle.color.textPrimary
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
                const validationError = root.computePortErrorText()
                portField.errorText = validationError
                if (validationError !== "") {
                    return
                }

                const value = portField.portValue()
                if (!LocalProxyController.setPort(value)) {
                    portField.errorText = root.portRangeErrorText()
                    return
                }
                PageController.showNotificationMessage(qsTr("Port saved: %1").arg(value))
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
            if (!LocalProxyController.isEnabled) {
                return
            }

            const activePort = LocalProxyController.activePort
            const requestedPort = root.pendingStartRequestedPort
            const shownPort = activePort > 0 ? activePort : requestedPort

            if (activePort > 0 && requestedPort > 0 && activePort !== requestedPort) {
                PageController.showNotificationMessage(qsTr("Port %1 is in use — selected free port %2")
                    .arg(requestedPort)
                    .arg(activePort))
            } else if (root.pendingStartVpnWasActive && shownPort > 0) {
                PageController.showNotificationMessage(qsTr("VPN off. Local proxy running: 127.0.0.1:%1")
                    .arg(shownPort))
            } else if (shownPort > 0) {
                PageController.showNotificationMessage(qsTr("Local proxy running: 127.0.0.1:%1")
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
        target: LocalProxyController

        function onStateChanged() {
            if (!portField.textField.activeFocus) {
                portField.syncPortValue()
            }
        }

        function onStartFailed(message) {
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
