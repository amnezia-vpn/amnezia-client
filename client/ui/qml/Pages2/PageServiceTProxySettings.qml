import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

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
    property bool isFetchingSecret: false
    property bool previousEnabled: true
    property int previousContainerStatus: 1
    property string previousHostname: ""
    property string previousEmail: ""
    property string previousSecret: ""
    property string savedHostname: ""
    property string savedCarrierMode: TProxyConfigModel.carrierModeHttps()
    property string savedWorkers: TProxyConfigModel.defaultWorkers()
    property bool pendingUpdateAfterEnable: false
    property bool remoteOperationBusy: false

    readonly property bool tproxyNetworkBlocked: !NetworkReachabilityController.hasInternetAccess
    readonly property bool pageBusy: isCheckingStatus || isFetchingSecret || isUpdating || remoteOperationBusy
    readonly property bool navigationBlockedWhileBusy: pageBusy

    property bool pageOpenHandled: false
    property bool busyIndicatorShown: false
    property bool containerStatusRefreshCallPending: false

    readonly property bool fieldsEditable: TProxyConfigModel.isEnabled && containerStatus === 1 && !root.pageBusy

    function effectiveHost() {
        return root.savedHostname !== "" ? root.savedHostname : TProxyConfigModel.hostname
    }

    function displayTmeLink() {
        if (TProxyConfigModel.tmeLink !== "")
            return TProxyConfigModel.tmeLink
        var host = effectiveHost()
        if (host === "" || TProxyConfigModel.secret === "")
            return ""
        return "https://telegram.me/webproxy?server=" + host + "&secret=" + TProxyConfigModel.secret
    }

    function displayTgLink() {
        if (TProxyConfigModel.tgLink !== "")
            return TProxyConfigModel.tgLink
        var host = effectiveHost()
        if (host === "" || TProxyConfigModel.secret === "")
            return ""
        return "tg://webproxy?server=" + host + "&secret=" + TProxyConfigModel.secret
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

    function tproxyRequestContainerStatusRefresh() {
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

    function tproxyScheduleContainerStatusRefresh() {
        if (containerStatusRefreshCallPending) {
            return
        }
        containerStatusRefreshCallPending = true
        Qt.callLater(function () {
            containerStatusRefreshCallPending = false
            root.tproxyRequestContainerStatusRefresh()
        })
    }

    function tproxyOnPageShown() {
        if (root.pageOpenHandled) {
            return
        }
        root.pageOpenHandled = true

        PageController.disableControls(navigationBlockedWhileBusy)

        if (!NetworkReachabilityController.hasInternetAccess) {
            isCheckingStatus = false
        } else {
            isCheckingStatus = true
        }
        syncPageBusyIndicator()
        root.tproxyScheduleContainerStatusRefresh()
    }

    function scheduleUpdate() {
        Qt.callLater(function () {
            InstallController.updateServerConfig(ServersUiController.processedServerId,
                ServersUiController.processedContainerIndex, ProtocolEnum.TProxy, false)
        })
    }

    Connections {
        target: InstallController

        function onUpdateContainerFinished(message, closePage) {
            if (!root.visible) {
                isUpdating = false
                isCheckingStatus = false
                isFetchingSecret = false
                return
            }
            isUpdating = false
            containerStatus = 1
            savedHostname = TProxyConfigModel.getHostname()
            savedCarrierMode = TProxyConfigModel.getCarrierMode()
            savedWorkers = TProxyConfigModel.getWorkers()
            PageController.showNotificationMessage(message)
        }
        function onInstallationErrorOccurred() {
            if (!root.visible) {
                isUpdating = false
                isCheckingStatus = false
                isFetchingSecret = false
                return
            }
            isUpdating = false
            isFetchingSecret = false
            containerStatus = previousContainerStatus
            TProxyConfigModel.setEnabled(previousEnabled)
            TProxyConfigModel.setHostname(previousHostname)
            TProxyConfigModel.setAcmeEmail(previousEmail)
            TProxyConfigModel.setCarrierMode(savedCarrierMode)
            TProxyConfigModel.setWorkers(savedWorkers)
            if (previousSecret !== "")
                TProxyConfigModel.setSecret(previousSecret)
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
            PageController.showNotificationMessage(enabled ? qsTr("TProxy started") : qsTr("TProxy stopped"))
        }
        function onServerIsBusy(busy) {
            root.remoteOperationBusy = busy
        }
        function onContainerStatusRefreshed(status, errorCode) {
            if (!root.visible) {
                isCheckingStatus = false
                isFetchingSecret = false
                return
            }
            containerStatus = status
            root.statusErrorCode = errorCode
            if (status === 3 && errorCode !== 0) {
                PageController.showNotificationMessage(
                    qsTr("Settings locked: connection timed out (error code %1). Re-open the page to retry.").arg(errorCode))
            }

            savedHostname = TProxyConfigModel.getHostname()
            savedCarrierMode = TProxyConfigModel.getCarrierMode()
            savedWorkers = TProxyConfigModel.getWorkers()
            if (status === 1) {
                TProxyConfigModel.setEnabled(true)
                isFetchingSecret = true
                isCheckingStatus = false
                InstallController.fetchContainerSecret(ServersUiController.processedServerId,
                    ServersUiController.processedContainerIndex)
            } else {
                isFetchingSecret = false
                isCheckingStatus = false
                if (status === 2) {
                    TProxyConfigModel.setEnabled(false)
                }
            }
            syncPageBusyIndicator()
        }
        function onContainerSecretFetched(value) {
            if (!root.visible) {
                isFetchingSecret = false
                return
            }
            isFetchingSecret = false
            syncPageBusyIndicator()
            if (value !== "")
                TProxyConfigModel.validateAndSetSecret(value)
        }
    }

    Component.onCompleted: {
        savedHostname = TProxyConfigModel.getHostname()
        savedCarrierMode = TProxyConfigModel.getCarrierMode()
        savedWorkers = TProxyConfigModel.getWorkers()
        Qt.callLater(root.tproxyOnPageShown)
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
            isFetchingSecret = false
            busyIndicatorShown = false
            statusErrorCode = 0
            PageController.disableControls(false)
            PageController.showBusyIndicator(false)
        } else {
            root.tproxyOnPageShown()
        }
    }

    Connections {
        target: NetworkReachabilityController

        function onHasInternetAccessChanged() {
            if (!root.visible) {
                return
            }
            if (NetworkReachabilityController.hasInternetAccess) {
                root.tproxyScheduleContainerStatusRefresh()
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

                title: qsTr("TProxy settings")
            }

            LinkButtonType {
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                size: LinkButtonType.Medium
                horizontalPadding: 14
                verticalPadding: 8

                text: qsTr("Read more about Telegram WEB proxy")

                onClicked: Qt.openUrlExternally("https://github.com/telegramdesktop/tproxy-server")
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.tproxyNetworkBlocked
                text: qsTr("No internet connection. Connect to the internet to change TProxy settings.")
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

                        text: qsTr("Use Telegram WEB proxy link")
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        implicitHeight: Math.max(72, linkRow.implicitHeight + 24)
                        color: AmneziaStyle.color.bgSurface
                        radius: 16

                        RowLayout {
                            id: linkRow

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 16
                            anchors.rightMargin: 12

                            spacing: 0

                            BodyTextType {
                                Layout.fillWidth: true

                                text: TProxyConfigModel.secret !== "" ? root.displayTmeLink() : qsTr("Set a hostname first")
                                color: TProxyConfigModel.secret !== "" ? AmneziaStyle.color.textPrimary : AmneziaStyle.color.textTertiary
                                wrapMode: Text.WrapAnywhere
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }

                            IconButtonType {
                                visible: TProxyConfigModel.secret !== ""
                                icon.source: "qrc:/images/controls/copy.svg"
                                Accessible.name: qsTr("Use Telegram WEB proxy link")

                                onClicked: {
                                    GC.copyToClipBoard(root.displayTmeLink())
                                    PageController.showNotificationMessage(qsTr("Copied"))
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        visible: TProxyConfigModel.secret !== ""
                        implicitHeight: Math.max(72, tgLinkRow.implicitHeight + 24)
                        color: AmneziaStyle.color.bgSurface
                        radius: 16

                        RowLayout {
                            id: tgLinkRow

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 16
                            anchors.rightMargin: 12

                            spacing: 0

                            BodyTextType {
                                Layout.fillWidth: true

                                text: root.displayTgLink()
                                color: AmneziaStyle.color.textPrimary
                                wrapMode: Text.WrapAnywhere
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }

                            IconButtonType {
                                icon.source: "qrc:/images/controls/qr-code.svg"
                                Accessible.name: qsTr("Telegram connection link")

                                onClicked: {
                                    ExportController.generateQrFromStringRaw(root.displayTgLink())
                                    PageController.goToShareConnectionPage(qsTr("Telegram connection link"),
                                        qsTr("TProxy WEB proxy link"), "", "", "")
                                }
                            }

                            IconButtonType {
                                icon.source: "qrc:/images/controls/copy.svg"

                                onClicked: {
                                    GC.copyToClipBoard(root.displayTgLink())
                                    PageController.showNotificationMessage(qsTr("Copied"))
                                }
                            }
                        }
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 24
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        text: qsTr("Or enter the proxy details manually.")
                    }

                    ListGroupType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        visible: TProxyConfigModel.secret !== ""

                        CopyableValueRow {
                            Layout.fillWidth: true

                            copiedText: qsTr("Copied")
                            label: qsTr("Host")
                            value: root.effectiveHost()
                        }

                        CopyableValueRow {
                            Layout.fillWidth: true

                            copiedText: qsTr("Copied")
                            label: qsTr("Secret")
                            value: TProxyConfigModel.secret
                            showSeparator: false
                        }
                    }

                    LinkButtonType {
                        Layout.topMargin: 12
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        size: LinkButtonType.Medium
                        horizontalPadding: 14
                        verticalPadding: 8

                        text: qsTr("How to do it")

                        onClicked: Qt.openUrlExternally("https://github.com/telegramdesktop/tproxy-server")
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        text: qsTr("Needs a WEB-capable Telegram client (Desktop proof of concept). Classic MTProxy links will not work.")
                    }

                    ButtonType {
                        id: removeButton

                        Layout.fillWidth: true
                        Layout.topMargin: 24
                        Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        visible: ServersUiController.isProcessedServerHasWriteAccess()
                        variant: ButtonType.Destructive
                        text: qsTr("Delete TProxy")

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
                        text: qsTr("Enable TProxy")
                        checked: TProxyConfigModel.isEnabled
                        enabled: containerStatus !== 0 && containerStatus !== 3 && !root.pageBusy && !root.tproxyNetworkBlocked
                        onToggled: function () {
                            if (checked !== TProxyConfigModel.isEnabled) {
                                previousEnabled = TProxyConfigModel.isEnabled
                                previousContainerStatus = containerStatus
                                root.previousSecret = TProxyConfigModel.secret
                                TProxyConfigModel.isEnabled = checked
                                isUpdating = true
                                if (checked) {
                                    root.pendingUpdateAfterEnable = true
                                    InstallController.setContainerEnabled(ServersUiController.processedServerId,
                                        ServersUiController.processedContainerIndex, true)
                                } else {
                                    InstallController.setContainerEnabled(ServersUiController.processedServerId,
                                        ServersUiController.processedContainerIndex, false)
                                }
                            }
                        }
                    }

                    SmallTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        visible: !root.fieldsEditable && !root.pageBusy
                        text: (containerStatus === 1 || containerStatus === 2)
                            ? qsTr("Enable TProxy to edit settings")
                            : (statusErrorCode !== 0
                                ? qsTr("Settings locked: connection timed out (error code %1). Re-open the page to retry.").arg(statusErrorCode)
                                : qsTr("Cannot reach the server — settings are unavailable"))
                    }

                    TextFieldType {
                        id: hostnameTextField
                        Layout.topMargin: 12
                        enabled: root.fieldsEditable
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        headerText: qsTr("Hostname")
                        textField.placeholderText: qsTr("proxy.example.com")
                        textField.text: TProxyConfigModel.hostname
                        textField.onTextChanged: {
                            var cur = hostnameTextField.textField.text
                            var clean = TProxyConfigModel.sanitizeHostnameFieldText(cur)
                            if (clean !== cur) {
                                textField.text = clean
                                textField.cursorPosition = clean.length
                                return
                            }
                            if (TProxyConfigModel.isHostnameTypingIncomplete(cur)) {
                                hostnameTextField.errorText = ""
                                return
                            }
                            if (!TProxyConfigModel.isValidHostname(clean)) {
                                hostnameTextField.errorText = qsTr("Use lowercase letters, digits, dots and hyphens")
                                return
                            }
                            hostnameTextField.errorText = ""
                        }
                        textField.onEditingFinished: {
                            var h = TProxyConfigModel.sanitizeHostnameFieldText(textField.text)
                            textField.text = h
                            if (!TProxyConfigModel.isValidHostname(h)) {
                                hostnameTextField.errorText = qsTr("Enter a lowercase DNS hostname (A record to this server)")
                                return
                            }
                            hostnameTextField.errorText = ""
                            TProxyConfigModel.setHostname(h)
                        }
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        text: qsTr("Required. Point a DNS A record at this server. Do not put a CDN in front.")
                    }

                    TextFieldType {
                        id: emailTextField
                        Layout.topMargin: 12
                        enabled: root.fieldsEditable
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        headerText: qsTr("ACME email")
                        textField.placeholderText: qsTr("you@example.com")
                        textField.text: TProxyConfigModel.acmeEmail
                        textField.onTextChanged: {
                            var cur = emailTextField.textField.text
                            var clean = TProxyConfigModel.sanitizeAcmeEmailFieldText(cur)
                            if (clean !== cur) {
                                textField.text = clean
                                textField.cursorPosition = clean.length
                                return
                            }
                            if (TProxyConfigModel.isAcmeEmailTypingIncomplete(cur)) {
                                emailTextField.errorText = ""
                                return
                            }
                            if (!TProxyConfigModel.isValidAcmeEmail(clean)) {
                                emailTextField.errorText = qsTr("Enter a valid email for the TLS certificate")
                                return
                            }
                            emailTextField.errorText = ""
                        }
                        textField.onEditingFinished: {
                            var e = TProxyConfigModel.sanitizeAcmeEmailFieldText(textField.text)
                            textField.text = e
                            if (!TProxyConfigModel.isValidAcmeEmail(e)) {
                                emailTextField.errorText = qsTr("Enter a valid email for the TLS certificate")
                                return
                            }
                            emailTextField.errorText = ""
                            TProxyConfigModel.setAcmeEmail(e)
                        }
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        text: qsTr("Used by Caddy to issue a Let's Encrypt certificate.")
                    }

                    DropDownType {
                        id: carrierDropDown
                        Layout.topMargin: 12
                        enabled: root.fieldsEditable
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        drawerHeight: 0.45
                        descriptionText: qsTr("Carrier mode")
                        headerText: qsTr("Carrier mode")
                        text: TProxyConfigModel.carrierModeLabel(TProxyConfigModel.carrierMode)
                        listView: Component {
                            ListViewType {
                                leftMargin: 16
                                rightMargin: 16

                                model: ["https", "https-lanes", "websocket", "websocket-lanes"]
                                delegate: ListItemType {
                                    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
                                    text: TProxyConfigModel.carrierModeLabel(modelData)
                                    trailingIconSource: modelData === TProxyConfigModel.carrierMode ? "qrc:/images/controls/check.svg" : ""
                                    onClicked: {
                                        TProxyConfigModel.setCarrierMode(modelData)
                                        carrierDropDown.close()
                                    }
                                }
                            }
                        }
                    }

                    TextFieldType {
                        id: workersTextField
                        enabled: root.fieldsEditable
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        headerText: qsTr("MTProxy workers")
                        textField.placeholderText: TProxyConfigModel.defaultWorkers()
                        textField.text: TProxyConfigModel.workers
                        textField.maximumLength: 2
                        textField.inputMethodHints: Qt.ImhDigitsOnly
                        textField.validator: IntValidator {
                            bottom: 1
                            top: TProxyConfigModel.maxWorkers()
                        }
                        textField.onTextChanged: {
                            var cur = workersTextField.textField.text
                            if (cur === "") {
                                return
                            }
                            var n = parseInt(cur, 10)
                            var maxW = TProxyConfigModel.maxWorkers()
                            if (isNaN(n) || n < 1) {
                                n = 1
                            }
                            if (n > maxW) {
                                n = maxW
                            }
                            var clamped = String(n)
                            if (clamped !== cur) {
                                textField.text = clamped
                                textField.cursorPosition = clamped.length
                            }
                        }
                        textField.onEditingFinished: {
                            var w = TProxyConfigModel.sanitizeWorkersFieldText(textField.text)
                            textField.text = w
                            TProxyConfigModel.setWorkers(w)
                        }
                    }

                    ButtonType {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        text: qsTr("Save")
                        enabled: root.fieldsEditable && !root.tproxyNetworkBlocked
                        onClicked: {
                            hostnameTextField.errorText = ""
                            emailTextField.errorText = ""

                            var h = TProxyConfigModel.sanitizeHostnameFieldText(hostnameTextField.textField.text)
                            var e = TProxyConfigModel.sanitizeAcmeEmailFieldText(emailTextField.textField.text)
                            hostnameTextField.textField.text = h
                            emailTextField.textField.text = e

                            var hasError = false

                            var httpsPortValue = TProxyConfigModel.defaultPort()
                            var httpPortValue = TProxyConfigModel.defaultHttpPort()

                            if (!TProxyConfigModel.isValidHostname(h)) {
                                hostnameTextField.errorText =
                                    qsTr("Enter a lowercase DNS hostname (A record to this server)")
                                hasError = true
                            }
                            if (!TProxyConfigModel.isValidAcmeEmail(e)) {
                                emailTextField.errorText = qsTr("Enter a valid email for the TLS certificate")
                                hasError = true
                            }
                            if (hasError) {
                                return
                            }

                            previousHostname = TProxyConfigModel.getHostname()
                            previousEmail = TProxyConfigModel.getAcmeEmail()
                            previousSecret = TProxyConfigModel.secret
                            TProxyConfigModel.setHostname(h)
                            TProxyConfigModel.setAcmeEmail(e)
                            TProxyConfigModel.setPort(httpsPortValue)
                            TProxyConfigModel.setHttpPort(httpPortValue)
                            TProxyConfigModel.setWorkers(TProxyConfigModel.sanitizeWorkersFieldText(workersTextField.textField.text))
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
