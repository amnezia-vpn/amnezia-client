import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0
import MtProxyConfig 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property int containerStatus: 1
    property int statusErrorCode: 0
    property bool isUpdating: false
    property bool isCheckingStatus: false
    property bool previousEnabled: true
    property int previousContainerStatus: 1
    property string savedPublicHost: ""
    property string savedPort: ""
    property string savedMediaPort: ""
    property bool pendingUpdateAfterEnable: false
    property bool remoteOperationBusy: false

    readonly property bool networkBlocked: !NetworkReachabilityController.hasInternetAccess
    readonly property bool pageBusy: isCheckingStatus || isUpdating || remoteOperationBusy
    readonly property bool navigationBlockedWhileBusy: pageBusy

    property bool pageOpenHandled: false
    property bool busyIndicatorShown: false
    property bool containerStatusRefreshCallPending: false

    readonly property bool fieldsEditable: WhatsAppProxyConfigModel.isEnabled && containerStatus === 1 && !root.pageBusy

    function effectiveHost() {
        return root.savedPublicHost !== "" ? root.savedPublicHost : ServersUiController.serverHostName(ServersUiController.processedServerId)
    }

    function shareText() {
        return qsTr("WhatsApp proxy") + "\n"
            + qsTr("Proxy address: %1").arg(root.effectiveHost()) + "\n"
            + qsTr("Chat port: %1 (TLS on)").arg(root.savedPort) + "\n"
            + qsTr("Media port: %1").arg(root.savedMediaPort) + "\n"
            + qsTr("WhatsApp: Settings > Storage and data > Proxy")
    }

    function saveCurrentValues() {
        savedPublicHost = WhatsAppProxyConfigModel.getPublicHost()
        savedPort = WhatsAppProxyConfigModel.getPort()
        savedMediaPort = WhatsAppProxyConfigModel.getMediaPort()
    }

    function syncPageBusyIndicator() {
        if (!root.pageOpenHandled) {
            return
        }
        var wantBusy = root.pageBusy
        if (wantBusy === root.busyIndicatorShown) {
            return
        }
        root.busyIndicatorShown = wantBusy
        PageController.showBusyIndicator(wantBusy)
    }

    onPageBusyChanged: syncPageBusyIndicator()

    function requestContainerStatusRefresh() {
        if (!NetworkReachabilityController.hasInternetAccess) {
            isCheckingStatus = false
            syncPageBusyIndicator()
            return
        }
        isCheckingStatus = true
        syncPageBusyIndicator()
        InstallController.refreshContainerStatus(ServersUiController.processedServerId,
            ServersUiController.processedContainerIndex)
    }

    function scheduleContainerStatusRefresh() {
        if (containerStatusRefreshCallPending) {
            return
        }
        containerStatusRefreshCallPending = true
        Qt.callLater(function () {
            containerStatusRefreshCallPending = false
            root.requestContainerStatusRefresh()
        })
    }

    function onPageShown() {
        if (root.pageOpenHandled) {
            return
        }
        root.pageOpenHandled = true

        PageController.disableControls(navigationBlockedWhileBusy)

        isCheckingStatus = NetworkReachabilityController.hasInternetAccess
        syncPageBusyIndicator()
        root.scheduleContainerStatusRefresh()
    }

    function scheduleUpdate() {
        Qt.callLater(function () {
            InstallController.updateServerConfig(ServersUiController.processedServerId,
                ServersUiController.processedContainerIndex, ProtocolEnum.WhatsAppProxy, false)
        })
    }

    Connections {
        target: InstallController

        function onUpdateContainerFinished(message, closePage) {
            if (!root.visible) {
                isUpdating = false
                isCheckingStatus = false
                return
            }
            isUpdating = false
            containerStatus = 1
            root.saveCurrentValues()
            PageController.showNotificationMessage(message)
        }
        function onInstallationErrorOccurred() {
            if (!root.visible) {
                isUpdating = false
                isCheckingStatus = false
                return
            }
            isUpdating = false
            containerStatus = previousContainerStatus
            WhatsAppProxyConfigModel.setEnabled(previousEnabled)
            WhatsAppProxyConfigModel.setPublicHost(savedPublicHost)
            WhatsAppProxyConfigModel.setPort(savedPort)
            WhatsAppProxyConfigModel.setMediaPort(savedMediaPort)
        }
        function onSetContainerEnabledFinished(enabled) {
            if (!root.visible) {
                isUpdating = false
                return
            }
            if (enabled && root.pendingUpdateAfterEnable) {
                root.pendingUpdateAfterEnable = false
                isUpdating = true
                root.scheduleUpdate()
                return
            }
            isUpdating = false
            containerStatus = enabled ? 1 : 2
            PageController.showNotificationMessage(enabled ? qsTr("WhatsApp proxy started") : qsTr("WhatsApp proxy stopped"))
        }
        function onServerIsBusy(busy) {
            root.remoteOperationBusy = busy
        }
        function onContainerStatusRefreshed(status, errorCode) {
            if (!root.visible) {
                isCheckingStatus = false
                return
            }
            containerStatus = status
            root.statusErrorCode = errorCode
            if (status === 3 && errorCode !== 0) {
                PageController.showNotificationMessage(
                    qsTr("Settings locked: connection timed out (error code %1). Re-open the page to retry.").arg(errorCode))
            }

            root.saveCurrentValues()
            if (status === 1) {
                WhatsAppProxyConfigModel.setEnabled(true)
            } else if (status === 2) {
                WhatsAppProxyConfigModel.setEnabled(false)
            }
            isCheckingStatus = false
            syncPageBusyIndicator()
        }
    }

    Component.onCompleted: {
        root.saveCurrentValues()
        Qt.callLater(root.onPageShown)
    }

    onNavigationBlockedWhileBusyChanged: {
        if (root.visible) {
            PageController.disableControls(navigationBlockedWhileBusy)
        }
    }

    onVisibleChanged: {
        if (!visible) {
            root.pageOpenHandled = false
            containerStatusRefreshCallPending = false
            isCheckingStatus = false
            busyIndicatorShown = false
            statusErrorCode = 0
            PageController.disableControls(false)
            PageController.showBusyIndicator(false)
        } else {
            root.onPageShown()
        }
    }

    Connections {
        target: NetworkReachabilityController

        function onHasInternetAccessChanged() {
            if (!root.visible) {
                return
            }
            if (NetworkReachabilityController.hasInternetAccess) {
                root.scheduleContainerStatusRefresh()
            }
        }
    }

    Item {
        id: contentLayer
        anchors.fill: parent
        enabled: !root.pageBusy

        ColumnLayout {
            id: pageHeader

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("WhatsApp proxy settings")
            }

            LinkButtonType {
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                size: LinkButtonType.Medium
                horizontalPadding: 14
                verticalPadding: 8

                text: qsTr("Read more about WhatsApp proxy")

                onClicked: Qt.openUrlExternally("https://github.com/WhatsApp/proxy")
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.networkBlocked
                text: qsTr("No internet connection. Connect to the internet to change WhatsApp proxy settings.")
            }
        }

        TabsType {
            id: mainTabBar

            anchors.top: pageHeader.bottom
            anchors.topMargin: 24
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            TabButtonType {
                text: qsTr("Connection")
            }
            TabButtonType {
                text: qsTr("Settings")
            }
        }

        StackLayout {
            anchors.top: mainTabBar.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            currentIndex: mainTabBar.currentIndex

            Item {
                FlickableType {
                    id: connectionFlickable

                    anchors.top: parent.top

                    contentHeight: connectionContent.implicitHeight

                    ColumnLayout {
                        id: connectionContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        spacing: 0

                        SmallTextType {
                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            text: qsTr("Enter these values in WhatsApp: Settings > Storage and data > Proxy")
                        }

                        ListGroupType {
                            Layout.fillWidth: true
                            Layout.topMargin: 12
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            CopyableValueRow {
                                Layout.fillWidth: true

                                copiedText: qsTr("Copied")
                                label: qsTr("Proxy address")
                                value: root.effectiveHost()
                            }

                            CopyableValueRow {
                                Layout.fillWidth: true

                                copiedText: qsTr("Copied")
                                label: qsTr("Chat port (TLS on)")
                                value: root.savedPort
                            }

                            CopyableValueRow {
                                Layout.fillWidth: true

                                copiedText: qsTr("Copied")
                                label: qsTr("Media port")
                                value: root.savedMediaPort
                                showSeparator: false
                            }
                        }

                        ButtonType {
                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            text: qsTr("Share")

                            onClicked: {
                                ExportController.generateQrFromStringRaw(root.shareText())
                                PageController.goToShareConnectionPage(qsTr("WhatsApp proxy"),
                                    qsTr("WhatsApp proxy settings"), "", ".txt", "whatsapp_proxy")
                            }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            Layout.topMargin: 12
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            text: qsTr("Anyone who knows the address and ports can use the proxy. Calls are not proxied.")
                        }

                        ButtonType {
                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            visible: ServersUiController.isProcessedServerHasWriteAccess()
                            variant: ButtonType.Destructive
                            text: qsTr("Delete WhatsApp proxy")

                            onClicked: {
                                var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                                var yesButtonFunction = function () {
                                    PageController.goToPage(PageEnum.PageDeinstalling)
                                    InstallController.removeContainer(ServersUiController.processedServerId,
                                        ServersUiController.processedContainerIndex)
                                }
                                Dialogs.askQuestion(headerText,
                                    qsTr("The proxy will be stopped and all users will lose access."),
                                    qsTr("Continue"), qsTr("Cancel"), yesButtonFunction, function () {})
                            }
                        }
                    }
                }
            }

            Item {
                FlickableType {
                    id: settingsFlickable

                    anchors.top: parent.top

                    contentHeight: settingsContent.implicitHeight

                    ColumnLayout {
                        id: settingsContent

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        spacing: 12

                        SwitchRowType {
                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            text: qsTr("Enable WhatsApp proxy")
                            checked: WhatsAppProxyConfigModel.isEnabled
                            enabled: containerStatus !== 0 && containerStatus !== 3 && !root.pageBusy && !root.networkBlocked
                            onToggled: function () {
                                if (checked !== WhatsAppProxyConfigModel.isEnabled) {
                                    previousEnabled = WhatsAppProxyConfigModel.isEnabled
                                    previousContainerStatus = containerStatus
                                    WhatsAppProxyConfigModel.isEnabled = checked
                                    isUpdating = true
                                    if (checked) {
                                        root.pendingUpdateAfterEnable = true
                                    }
                                    InstallController.setContainerEnabled(ServersUiController.processedServerId,
                                        ServersUiController.processedContainerIndex, checked)
                                }
                            }
                        }

                        SmallTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            visible: !root.fieldsEditable && !root.pageBusy
                            text: (containerStatus === 1 || containerStatus === 2)
                                ? qsTr("Enable WhatsApp proxy to edit settings")
                                : (statusErrorCode !== 0
                                    ? qsTr("Settings locked: connection timed out (error code %1). Re-open the page to retry.").arg(statusErrorCode)
                                    : qsTr("Cannot reach the server — settings are unavailable"))
                        }

                        TextFieldType {
                            id: publicHostTextField
                            Layout.topMargin: 12
                            enabled: root.fieldsEditable
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            headerText: qsTr("Public host / IP")
                            textField.placeholderText: ServersUiController.serverHostName(ServersUiController.processedServerId)
                            textField.text: WhatsAppProxyConfigModel.publicHost
                            textField.maximumLength: 253
                            textField.validator: PublicHostInputValidator {
                            }
                            textField.onEditingFinished: {
                                textField.text = textField.text.trim()
                                if (!WhatsAppProxyConfigModel.isValidPublicHost(textField.text)) {
                                    publicHostTextField.errorText = qsTr("Enter a valid IP address or domain name")
                                    return
                                }
                                publicHostTextField.errorText = ""
                            }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            text: qsTr("Leave empty to use server IP automatically")
                        }

                        TextFieldType {
                            id: portTextField
                            enabled: root.fieldsEditable
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            headerText: qsTr("Chat port")
                            textField.placeholderText: WhatsAppProxyConfigModel.defaultPort()
                            textField.text: WhatsAppProxyConfigModel.port
                            textField.maximumLength: 5
                            textField.inputMethodHints: Qt.ImhDigitsOnly
                            textField.validator: IntValidator { bottom: 1; top: 65535 }
                        }

                        TextFieldType {
                            id: mediaPortTextField
                            enabled: root.fieldsEditable
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            headerText: qsTr("Media port")
                            textField.placeholderText: WhatsAppProxyConfigModel.defaultMediaPort()
                            textField.text: WhatsAppProxyConfigModel.mediaPort
                            textField.maximumLength: 5
                            textField.inputMethodHints: Qt.ImhDigitsOnly
                            textField.validator: IntValidator { bottom: 1; top: 65535 }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            text: qsTr("Changing a port reinstalls the container. WhatsApp uses 443 and 587 by default; with other ports users enter them manually and enable TLS for the chat port.")
                        }

                        ButtonType {
                            Layout.fillWidth: true
                            Layout.topMargin: 12
                            Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            text: qsTr("Save")
                            enabled: root.fieldsEditable && !root.networkBlocked
                            onClicked: {
                                publicHostTextField.errorText = ""
                                portTextField.errorText = ""
                                mediaPortTextField.errorText = ""

                                var host = publicHostTextField.textField.text.trim()
                                var chatPort = WhatsAppProxyConfigModel.sanitizePortFieldText(portTextField.textField.text)
                                var mediaPort = WhatsAppProxyConfigModel.sanitizePortFieldText(mediaPortTextField.textField.text)

                                var hasError = false
                                if (!WhatsAppProxyConfigModel.isValidPublicHost(host)) {
                                    publicHostTextField.errorText = qsTr("Enter a valid IP address or domain name")
                                    hasError = true
                                }
                                if (!WhatsAppProxyConfigModel.isValidPort(chatPort)) {
                                    portTextField.errorText = qsTr("The port must be in the range of 1 to 65535")
                                    hasError = true
                                }
                                if (!WhatsAppProxyConfigModel.isValidPort(mediaPort)) {
                                    mediaPortTextField.errorText = qsTr("The port must be in the range of 1 to 65535")
                                    hasError = true
                                } else if (mediaPort === chatPort) {
                                    mediaPortTextField.errorText = qsTr("The media port must differ from the chat port")
                                    hasError = true
                                }
                                if (hasError) {
                                    return
                                }

                                previousContainerStatus = containerStatus
                                WhatsAppProxyConfigModel.setPublicHost(host)
                                WhatsAppProxyConfigModel.setPort(chatPort)
                                WhatsAppProxyConfigModel.setMediaPort(mediaPort)
                                isUpdating = true
                                root.scheduleUpdate()
                            }
                        }
                    }
                }
            }
        }
    }
}
