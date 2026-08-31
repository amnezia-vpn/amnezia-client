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

        BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onFocusChanged: {
            if (this.activeFocus) {
                if (mainTabBar.currentIndex === 0) {
                    connectionListView.positionViewAtBeginning()
                } else {
                    settingsListView.positionViewAtBeginning()
                }
            }
        }
    }

    ColumnLayout {
        id: pageHeader
        anchors.top: backButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        BaseHeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 24
            headerText: qsTr("TProxy settings")
            descriptionLinkText: qsTr("Read more about Telegram WEB proxy")
            descriptionLinkUrl: "https://github.com/telegramdesktop/tproxy-server"
        }

        CaptionTextType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 8
            visible: root.tproxyNetworkBlocked
            text: qsTr("No internet connection. Connect to the internet to change TProxy settings.")
            color: AmneziaStyle.color.mutedGray
            wrapMode: Text.WordWrap
            font.pixelSize: 14
        }
    }

    TabBar {
        id: mainTabBar
        anchors.top: pageHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        width: parent.width
        background: Rectangle {
            color: AmneziaStyle.color.transparent
            Rectangle {
                width: parent.width
                height: 1
                anchors.bottom: parent.bottom
                color: AmneziaStyle.color.slateGray
            }
        }
        TabButtonType {
            text: qsTr("Connection")
            isSelected: mainTabBar.currentIndex === 0
        }
        TabButtonType {
            text: qsTr("Settings")
            isSelected: mainTabBar.currentIndex === 1
        }
    }

    StackLayout {
        anchors.top: mainTabBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: mainTabBar.currentIndex

        ListViewType {
            id: connectionListView
            model: TProxyConfigModel
            delegate: ColumnLayout {
                width: connectionListView.width
                spacing: 0

                function effectiveHost() {
                    return root.savedHostname !== "" ? root.savedHostname : hostname
                }

                function displayTmeLink() {
                    if (tmeLink !== "")
                        return tmeLink
                    var host = effectiveHost()
                    if (host === "" || secret === "")
                        return ""
                    return "https://t.me/webproxy?server=" + host + "&secret=" + secret
                }

                function displayTgLink() {
                    if (tgLink !== "")
                        return tgLink
                    var host = effectiveHost()
                    if (host === "" || secret === "")
                        return ""
                    return "tg://webproxy?server=" + host + "&secret=" + secret
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("Use Telegram WEB proxy link")
                    color: AmneziaStyle.color.mutedGray
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    implicitHeight: linkRow.implicitHeight + 16
                    color: AmneziaStyle.color.onyxBlack
                    radius: 8
                    border.color: AmneziaStyle.color.slateGray
                    border.width: 1

                    RowLayout {
                        id: linkRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 12
                        anchors.rightMargin: 8
                        spacing: 4

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: secret !== "" ? displayTmeLink() : qsTr("Set a hostname first")
                            color: secret !== "" ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.mutedGray
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            font.pixelSize: 13
                        }

                        ImageButtonType {
                            implicitWidth: 36
                            implicitHeight: 36
                            hoverEnabled: true
                            image: "qrc:/images/controls/copy.svg"
                            imageColor: AmneziaStyle.color.paleGray
                            visible: secret !== ""
                            onClicked: {
                                GC.copyToClipBoard(displayTmeLink())
                                PageController.showNotificationMessage(qsTr("Copied"))
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    implicitHeight: tgLinkRow.implicitHeight + 16
                    color: AmneziaStyle.color.onyxBlack
                    radius: 8
                    border.color: AmneziaStyle.color.slateGray
                    border.width: 1
                    visible: secret !== ""

                    RowLayout {
                        id: tgLinkRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 12
                        anchors.rightMargin: 8
                        spacing: 4

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: displayTgLink()
                            color: AmneziaStyle.color.goldenApricot
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            font.pixelSize: 13
                        }

                        ImageButtonType {
                            implicitWidth: 36
                            implicitHeight: 36
                            hoverEnabled: true
                            image: "qrc:/images/controls/qr-code.svg"
                            imageColor: AmneziaStyle.color.paleGray
                            onClicked: {
                                ExportController.generateQrFromStringRaw(displayTgLink())
                                PageController.goToShareConnectionPage(qsTr("Telegram connection link"),
                                    qsTr("TProxy WEB proxy link"), "", "", "")
                            }
                        }

                        ImageButtonType {
                            implicitWidth: 36
                            implicitHeight: 36
                            hoverEnabled: true
                            image: "qrc:/images/controls/copy.svg"
                            imageColor: AmneziaStyle.color.paleGray
                            onClicked: {
                                GC.copyToClipBoard(displayTgLink())
                                PageController.showNotificationMessage(qsTr("Copied"))
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    spacing: 4

                    CaptionTextType {
                        text: qsTr("Or enter the proxy details manually.")
                        color: AmneziaStyle.color.mutedGray
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("How to do it")
                        color: AmneziaStyle.color.goldenApricot
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.openUrlExternally("https://github.com/telegramdesktop/tproxy-server")
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    implicitHeight: manualCol.implicitHeight + 8
                    color: AmneziaStyle.color.onyxBlack
                    radius: 8
                    border.color: AmneziaStyle.color.slateGray
                    border.width: 1
                    visible: secret !== ""

                    ColumnLayout {
                        id: manualCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: 8
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 8
                            Layout.bottomMargin: 8
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                CaptionTextType {
                                    text: qsTr("Host")
                                    color: AmneziaStyle.color.mutedGray
                                    font.pixelSize: 12
                                }
                                CaptionTextType {
                                    Layout.fillWidth: true
                                    text: effectiveHost()
                                    color: AmneziaStyle.color.paleGray
                                    elide: Text.ElideRight
                                }
                            }
                            ImageButtonType {
                                implicitWidth: 36
                                implicitHeight: 36
                                hoverEnabled: true
                                image: "qrc:/images/controls/copy.svg"
                                imageColor: AmneziaStyle.color.paleGray
                                onClicked: {
                                    GC.copyToClipBoard(effectiveHost())
                                    PageController.showNotificationMessage(qsTr("Copied"))
                                }
                            }
                        }

                        DividerType {
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 8
                            Layout.topMargin: 8
                            Layout.bottomMargin: 8
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                CaptionTextType {
                                    text: qsTr("Secret")
                                    color: AmneziaStyle.color.mutedGray
                                    font.pixelSize: 12
                                }
                                CaptionTextType {
                                    Layout.fillWidth: true
                                    text: secret
                                    color: AmneziaStyle.color.paleGray
                                    wrapMode: Text.WrapAnywhere
                                    font.pixelSize: 13
                                }
                            }
                            ImageButtonType {
                                implicitWidth: 36
                                implicitHeight: 36
                                hoverEnabled: true
                                image: "qrc:/images/controls/copy.svg"
                                imageColor: AmneziaStyle.color.paleGray
                                onClicked: {
                                    GC.copyToClipBoard(secret)
                                    PageController.showNotificationMessage(qsTr("Copied"))
                                }
                            }
                        }
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    Layout.bottomMargin: 16
                    text: qsTr("Needs a WEB-capable Telegram client (Desktop proof of concept). Classic MTProxy links will not work.")
                    color: AmneziaStyle.color.mutedGray
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                }

                LabelWithButtonType {
                    id: removeButton
                    Layout.fillWidth: true
                    Layout.bottomMargin: 24
                    Layout.leftMargin: 0
                    Layout.rightMargin: 16
                    visible: ServersUiController.isProcessedServerHasWriteAccess()
                    text: qsTr("Delete TProxy")
                    textColor: AmneziaStyle.color.vibrantRed
                    clickedFunction: function () {
                        var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                        var yesButtonFunction = function () {
                            PageController.goToPage(PageEnum.PageDeinstalling)
                            InstallController.removeContainer(ServersUiController.processedServerId,
                                ServersUiController.processedContainerIndex)
                        }
                        showQuestionDrawer(headerText,
                            qsTr("The proxy will be stopped and all users will lose access."),
                            qsTr("Continue"), qsTr("Cancel"), yesButtonFunction, function () {})
                    }
                }
            }
        }

        ListViewType {
            id: settingsListView
            model: TProxyConfigModel
            delegate: ColumnLayout {
                width: settingsListView.width
                spacing: 0
                readonly property bool fieldsEditable: isEnabled && containerStatus === 1 && !root.pageBusy

                SwitcherType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    text: qsTr("Enable TProxy")
                    checked: isEnabled
                    enabled: containerStatus !== 0 && containerStatus !== 3 && !root.pageBusy && !root.tproxyNetworkBlocked
                    onToggled: function () {
                        if (checked !== isEnabled) {
                            previousEnabled = isEnabled
                            previousContainerStatus = containerStatus
                            root.previousSecret = secret
                            isEnabled = checked
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

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    visible: !fieldsEditable && !root.pageBusy
                    text: (containerStatus === 1 || containerStatus === 2)
                        ? qsTr("Enable TProxy to edit settings")
                        : (statusErrorCode !== 0
                            ? qsTr("Settings locked: connection timed out (error code %1). Re-open the page to retry.").arg(statusErrorCode)
                            : qsTr("Cannot reach the server — settings are unavailable"))
                    color: AmneziaStyle.color.mutedGray
                    wrapMode: Text.WordWrap
                }

                TextFieldWithHeaderType {
                    id: hostnameTextField
                    enabled: fieldsEditable
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    headerText: qsTr("Hostname")
                    textField.placeholderText: qsTr("proxy.example.com")
                    textField.text: hostname
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
                    Layout.bottomMargin: 16
                    text: qsTr("Required. Point a DNS A record at this server. Do not put a CDN in front.")
                    color: AmneziaStyle.color.mutedGray
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                }

                TextFieldWithHeaderType {
                    id: emailTextField
                    enabled: fieldsEditable
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    headerText: qsTr("ACME email")
                    textField.placeholderText: qsTr("you@example.com")
                    textField.text: acmeEmail
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
                    Layout.bottomMargin: 16
                    text: qsTr("Used by Caddy to issue a Let's Encrypt certificate.")
                    color: AmneziaStyle.color.mutedGray
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: qsTr("Carrier mode")
                    color: AmneziaStyle.color.mutedGray
                    font.pixelSize: 12
                }

                DropDownType {
                    id: carrierDropDown
                    enabled: fieldsEditable
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    drawerParent: root
                    drawerHeight: 0.45
                    headerText: qsTr("Carrier mode")
                    text: TProxyConfigModel.carrierModeLabel(carrierMode)
                    listView: Component {
                        ListViewType {
                            model: ["https", "https-lanes", "websocket", "websocket-lanes"]
                            delegate: LabelWithButtonType {
                                Layout.fillWidth: true
                                text: TProxyConfigModel.carrierModeLabel(modelData)
                                rightImageSource: modelData === carrierMode ? "qrc:/images/controls/check.svg" : ""
                                rightImageColor: AmneziaStyle.color.goldenApricot
                                clickedFunction: function () {
                                    TProxyConfigModel.setCarrierMode(modelData)
                                    carrierDropDown.closeTriggered()
                                }
                            }
                        }
                    }
                }

                TextFieldWithHeaderType {
                    id: workersTextField
                    enabled: fieldsEditable
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    headerText: qsTr("MTProxy workers")
                    textField.placeholderText: TProxyConfigModel.defaultWorkers()
                    textField.text: workers
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

                BasicButtonType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 32
                    text: qsTr("Save")
                    enabled: fieldsEditable && !root.tproxyNetworkBlocked
                    clickedFunc: function () {
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
                        previousSecret = secret
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
