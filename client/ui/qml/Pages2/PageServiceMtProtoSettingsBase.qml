import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
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

    property QtObject configModel
    property bool isTelemt: false
    property int protocol
    property string pageTitle
    property string readMoreUrl
    property string noInternetText
    property string startedText
    property string stoppedText
    property string deployFirstText
    property string connectionLinkText
    property string deleteText
    property string enableText
    property string enableToEditText
    property string newSecretSavedText
    property string natSwitchText
    property string natSwitchDescriptionText
    property string natExternalIpHeaderText
    property string natExternalIpErrorText

    property int containerStatus: 1
    property int statusErrorCode: 0
    property bool isUpdating: false
    property bool isCheckingStatus: false
    property bool isFetchingSecret: false
    property bool previousEnabled: true
    property int previousContainerStatus: 1

    property string previousPort: ""
    property string previousTag: ""
    property string previousPublicHost: ""
    property string previousTransportMode: root.configModel.transportModeStandard()
    property string previousTlsDomain: root.configModel.defaultTlsDomain()
    property string previousWorkersMode: root.configModel.workersModeAuto()
    property string previousWorkers: root.configModel.defaultWorkers()
    property bool   previousNatEnabled: false
    property string previousNatInternalIp: ""
    property string previousNatExternalIp: ""
    property string previousSecret: ""

    property string savedTransportMode: ""
    property string savedTlsDomain: ""
    property string savedPublicHost: ""

    onSavedTransportModeChanged: {
        if (savedTransportMode === "faketls") {
            root.syncedSecretTabIndex = 1
        } else if (savedTransportMode !== "") {
            root.syncedSecretTabIndex = 0
        }
    }

    property bool diagLoading: false
    property int syncedSecretTabIndex: 0
    property bool pendingEnableAfterRestart: false
    property bool pendingUpdateAfterEnable: false
    property bool diagPortReachable: false
    property bool diagTelegramReachable: false
    property int  diagClientsConnected: -1
    property string diagLastConfigRefresh: ""
    property string diagStatsEndpoint: ""

    readonly property bool networkBlocked: !NetworkReachabilityController.hasInternetAccess

    property bool remoteOperationBusy: false
    readonly property bool operationInProgress: isCheckingStatus || isFetchingSecret || isUpdating || diagLoading
    readonly property bool pageBusy: operationInProgress || remoteOperationBusy
    readonly property bool navigationBlockedWhileBusy: pageBusy

    property bool pageOpenHandled: false
    property bool busyIndicatorShown: false

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

    function domainToHex(domain) {
        var hex = ""
        for (var i = 0; i < domain.length; i++) {
            var code = domain.charCodeAt(i).toString(16)
            hex += (code.length < 2 ? "0" : "") + code
        }
        return hex
    }

    function clientSecret(baseHex32, mode, tlsDomain) {
        if (baseHex32 === "") {
            return ""
        }
        if (mode === "faketls") {
            return "ee" + baseHex32 + domainToHex(tlsDomain)
        }
        return "dd" + baseHex32
    }

    function clientSecretForTabIndex(baseHex32, tabIndex, tlsDomain, defaultTlsDomain) {
        var domain = tlsDomain !== "" ? tlsDomain : defaultTlsDomain
        if (tabIndex === 1) {
            return clientSecret(baseHex32, "faketls", domain)
        }
        return clientSecret(baseHex32, "standard", domain)
    }

    property bool containerStatusRefreshCallPending: false

    function requestContainerStatusRefresh() {
        if (!NetworkReachabilityController.hasInternetAccess) {
            isCheckingStatus = false
            syncPageBusyIndicator()
            return
        }
        isCheckingStatus = true
        syncPageBusyIndicator()
        InstallController.refreshContainerStatus(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
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

    function handlePageShown() {
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
        root.scheduleContainerStatusRefresh()
    }

    property var persistedAdditionalHex: []

    function refreshPersistedAdditionalSecrets() {
        var list = root.configModel.additionalSecretsList()
        var a = []
        for (var i = 0; i < list.length; ++i) {
            a.push(String(list[i]))
        }
        root.persistedAdditionalHex = a
    }

    function isPersistedAdditionalHex(hex) {
        var h = String(hex)
        for (var j = 0; j < root.persistedAdditionalHex.length; ++j) {
            if (String(root.persistedAdditionalHex[j]) === h) {
                return true
            }
        }
        return false
    }

    readonly property bool fieldsEditable: root.configModel.isEnabled && containerStatus === 1 && !root.pageBusy

    function activeSecretForBaseHex(baseHex) {
        return root.clientSecretForTabIndex(baseHex, root.syncedSecretTabIndex,
            root.savedTlsDomain, root.configModel.defaultTlsDomain())
    }

    function activeSecret() {
        return root.activeSecretForBaseHex(root.configModel.secret)
    }

    function effectiveHost() {
        return root.savedPublicHost !== "" ? root.savedPublicHost : ServersUiController.serverHostName(ServersUiController.processedServerId)
    }

    function tmeLinkFor(baseHex) {
        return "https://telegram.me/proxy?server=" + root.effectiveHost() + "&port=" + root.configModel.port + "&secret=" + root.activeSecretForBaseHex(baseHex)
    }

    function tgLinkFor(baseHex) {
        return "tg://proxy?server=" + root.effectiveHost() + "&port=" + root.configModel.port + "&secret=" + root.activeSecretForBaseHex(baseHex)
    }

    function tmeLink() {
        return root.tmeLinkFor(root.configModel.secret)
    }

    function tgLink() {
        return root.tgLinkFor(root.configModel.secret)
    }

    function copyText(text) {
        GC.copyToClipBoard(text)
        PageController.showNotificationMessage(qsTr("Copied"))
    }

    function shareQr(link) {
        ExportController.generateQrFromStringRaw(link)
        PageController.goToShareConnectionPage(qsTr("Telegram connection link"),
            root.connectionLinkText, "", "", "")
    }

    function loadFieldsFromModel() {
        publicHostTextField.textField.text = root.configModel.publicHost
        var savedPort = root.configModel.port
        portTextField.textField.text = (savedPort === root.configModel.defaultPort()) ? "" : savedPort
        tagTextField.textField.text = root.configModel.tag
        var savedDomain = root.configModel.tlsDomain
        tlsDomainTextField.textField.text = (savedDomain === root.configModel.defaultTlsDomain() || savedDomain === "") ? "" : savedDomain
        workersTextField.textField.text = root.configModel.workers
        natInternalIpTextField.textField.text = root.configModel.natInternalIp
        natExternalIpTextField.textField.text = root.configModel.natExternalIp
    }

    readonly property var natIpv4InputFormat: /^(\d{1,3}\.){0,3}\d{0,3}$/

    function scheduleUpdate(closePage) {
        var cp = closePage === undefined ? false : closePage
        Qt.callLater(function () {
            InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, root.protocol, cp)
        })
    }

    function natIpv4FieldShowInvalidError(text) {
        var t = text ? String(text).replace(/^\s+|\s+$/g, '') : ""
        if (t === "")
            return false
        if (root.configModel.isValidOptionalIpv4(t))
            return false
        var parts = t.split('.')
        var j
        for (j = 0; j < parts.length; j++) {
            if (parts[j].length > 3)
                return true
        }
        if (parts.length > 4)
            return true
        if (t.indexOf('.') < 0 && t.length > 3)
            return true
        if (t.endsWith('.'))
            return false
        if (parts.length < 4)
            return false
        for (var i = 0; i < parts.length; i++) {
            if (parts[i] === "")
                return true
        }
        return true
    }

    function statusText() {
        if (isCheckingStatus) {
            return qsTr("Checking...")
        }
        if (isUpdating) {
            return qsTr("Updating")
        }
        switch (containerStatus) {
            case 0: {
                return qsTr("Not deployed")
            }
            case 1: {
                return qsTr("Running")
            }
            case 2: {
                return qsTr("Stopped")
            }
            case 3: {
                return qsTr("Error")
            }
            default: {
                return qsTr("Unknown")
            }
        }
    }

    Component.onCompleted: {
        root.loadFieldsFromModel()
        root.savedTransportMode = root.configModel.getTransportMode()
        root.savedTlsDomain = root.configModel.getTlsDomain()
        root.savedPublicHost = root.configModel.getPublicHost()

        Qt.callLater(function () {
            root.refreshPersistedAdditionalSecrets()
        })

        Qt.callLater(root.handlePageShown)
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
            diagLoading = false
        } else {
            root.handlePageShown()
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

    Connections {
        target: InstallController

        function onServerIsBusy(busy) {
            remoteOperationBusy = busy
        }

        function onUpdateContainerFinished(message, closePage) {
            if (!root.visible) {
                isUpdating = false
                isCheckingStatus = false
                isFetchingSecret = false
                return
            }
            isUpdating = false
            containerStatus = 1
            root.savedTransportMode = root.configModel.getTransportMode()
            root.savedTlsDomain = root.configModel.getTlsDomain()
            root.savedPublicHost = root.configModel.getPublicHost()
            root.refreshPersistedAdditionalSecrets()
            root.loadFieldsFromModel()
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
            root.configModel.setEnabled(previousEnabled)
            root.configModel.setPort(previousPort)
            root.configModel.setTag(previousTag)
            root.configModel.setPublicHost(previousPublicHost)
            root.configModel.setTransportMode(previousTransportMode)
            root.configModel.setTlsDomain(previousTlsDomain)
            root.configModel.setWorkersMode(previousWorkersMode)
            root.configModel.setWorkers(previousWorkers)
            root.configModel.setNatEnabled(previousNatEnabled)
            root.configModel.setNatInternalIp(previousNatInternalIp)
            root.configModel.setNatExternalIp(previousNatExternalIp)
            if (previousSecret !== "") {
                root.configModel.setSecret(previousSecret)
            }
        }

        function onSetContainerEnabledFinished(enabled) {
            if (!root.visible) {
                isUpdating = false
                return
            }
            if (enabled && pendingUpdateAfterEnable) {
                pendingUpdateAfterEnable = false
                isUpdating = true
                root.scheduleUpdate(false)
                return
            }
            isUpdating = false
            containerStatus = enabled ? 1 : 2
            PageController.showNotificationMessage(
                enabled ? root.startedText : root.stoppedText)
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

            root.savedTransportMode = root.configModel.getTransportMode()
            root.savedTlsDomain = root.configModel.getTlsDomain()
            root.savedPublicHost = root.configModel.getPublicHost()
            if (status === 1) {
                root.configModel.setEnabled(true)
                isFetchingSecret = true
                isCheckingStatus = false
                InstallController.fetchContainerSecret(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
            } else {
                isFetchingSecret = false
                isCheckingStatus = false
                if (status === 2) {
                    root.configModel.setEnabled(false)
                }
            }
            syncPageBusyIndicator()
        }

        function onContainerDiagnosticsRefreshed(portReachable, upstreamReachable, clientsConnected, lastConfigRefresh, statsEndpoint) {
            if (!root.visible) {
                return
            }
            diagLoading = false
            diagPortReachable = portReachable
            diagTelegramReachable = upstreamReachable
            diagClientsConnected = clientsConnected
            diagLastConfigRefresh = lastConfigRefresh
            diagStatsEndpoint = statsEndpoint
        }

        function onContainerSecretFetched(secret) {
            if (!root.visible) {
                isFetchingSecret = false
                return
            }
            isFetchingSecret = false
            syncPageBusyIndicator()
            root.configModel.validateAndSetSecret(secret)
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

                title: root.pageTitle
            }

            LinkButtonType {
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                size: LinkButtonType.Medium
                horizontalPadding: 14
                verticalPadding: 8

                text: qsTr("Read more about this settings")

                onClicked: Qt.openUrlExternally(root.readMoreUrl)
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.networkBlocked
                text: root.noInternetText
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
            id: tabContent
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

                            text: qsTr("Use Telegram connection link")
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

                                    text: root.configModel.secret !== "" ? root.tmeLink() : root.deployFirstText
                                    color: root.configModel.secret !== "" ? AmneziaStyle.color.textPrimary : AmneziaStyle.color.textTertiary
                                    wrapMode: Text.WrapAnywhere
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }

                                IconButtonType {
                                    visible: root.configModel.secret !== ""
                                    icon.source: "qrc:/images/controls/copy.svg"
                                    Accessible.name: qsTr("Use Telegram connection link")

                                    onClicked: {
                                        GC.copyToClipBoard(root.tmeLink())
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

                            visible: root.configModel.secret !== ""
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

                                    text: root.tgLink()
                                    color: AmneziaStyle.color.textPrimary
                                    wrapMode: Text.WrapAnywhere
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }

                                IconButtonType {
                                    icon.source: "qrc:/images/controls/qr-code.svg"
                                    Accessible.name: qsTr("Telegram connection link")

                                    onClicked: {
                                        ExportController.generateQrFromStringRaw(root.tgLink())
                                        PageController.goToShareConnectionPage(
                                            qsTr("Telegram connection link"),
                                            root.connectionLinkText,
                                            "", "", "")
                                    }
                                }

                                IconButtonType {
                                    icon.source: "qrc:/images/controls/copy.svg"

                                    onClicked: {
                                        GC.copyToClipBoard(root.tgLink())
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

                            CopyableValueRow {
                                Layout.fillWidth: true

                                copiedText: qsTr("Copied")
                                label: qsTr("Host")
                                value: root.effectiveHost()
                            }

                            CopyableValueRow {
                                Layout.fillWidth: true

                                copiedText: qsTr("Copied")
                                label: qsTr("Port")
                                value: root.configModel.port
                            }

                            CopyableValueRow {
                                Layout.fillWidth: true

                                copiedText: qsTr("Copied")
                                label: qsTr("Secret")
                                value: root.activeSecret()
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

                            onClicked: Qt.openUrlExternally("https://core.telegram.org/proxy")
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
                            text: root.deleteText

                            onClicked: {
                                var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                                var descriptionText = qsTr("The proxy will be stopped and all users will lose access.")
                                var yesButtonText = qsTr("Continue")
                                var noButtonText = qsTr("Cancel")
                                var yesButtonFunction = function () {
                                    PageController.goToPage(PageEnum.PageDeinstalling)
                                    InstallController.removeContainer(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
                                }
                                Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, function () {
                                })
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
                            id: enableSwitch
                            Layout.topMargin: 24
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            text: root.enableText
                            checked: root.configModel.isEnabled
                            enabled: containerStatus !== 0 && containerStatus !== 3 && !root.pageBusy
                                && !root.networkBlocked
                            onToggled: function () {
                                if (checked !== root.configModel.isEnabled) {
                                    previousEnabled = root.configModel.isEnabled
                                    previousContainerStatus = containerStatus
                                    root.previousSecret = root.configModel.secret
                                    root.configModel.isEnabled = checked
                                    isUpdating = true
                                    if (checked) {
                                        root.pendingUpdateAfterEnable = true
                                        InstallController.setContainerEnabled(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, true)
                                    } else {
                                        InstallController.setContainerEnabled(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, false)
                                    }
                                }
                            }
                        }

                        SmallTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            visible: !fieldsEditable && !root.pageBusy
                            text: (containerStatus === 1 || containerStatus === 2)
                                ? root.enableToEditText
                                : (statusErrorCode !== 0
                                    ? qsTr("Settings locked: connection timed out (error code %1). Re-open the page to retry.").arg(statusErrorCode)
                                    : qsTr("Cannot reach the server — settings are unavailable"))
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.topMargin: 12
                            spacing: 4

                            SmallTextType {
                                text: qsTr("Base secret")
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                SmallTextType {
                                    Layout.fillWidth: true
                                    text: root.configModel.secret !== "" ? root.activeSecretForBaseHex(root.configModel.secret) : qsTr("Not generated")
                                    color: root.configModel.secret !== "" ? AmneziaStyle.color.textSecondary : AmneziaStyle.color.textTertiary
                                    wrapMode: Text.WrapAnywhere
                                }

                                IconButtonType {
                                    Layout.alignment: Qt.AlignVCenter
                                    icon.source: "qrc:/images/controls/refresh-cw.svg"
                                    visible: ServersUiController.isProcessedServerHasWriteAccess()
                                    enabled: fieldsEditable
                                    onClicked: {
                                        var secretSnapshot = root.configModel.secret
                                        Dialogs.askQuestion(
                                            qsTr("Generate new secret?"),
                                            qsTr("All existing connection links will stop working. Users will need new links."),
                                            qsTr("Generate"),
                                            qsTr("Cancel"),
                                                function () {
                                                root.previousSecret = secretSnapshot
                                                if (containerStatus === 1) {
                                                    isUpdating = true
                                                    root.configModel.generateSecret()
                                                    root.scheduleUpdate(false)
                                                } else {
                                                    root.configModel.generateSecret()
                                                    PageController.showNotificationMessage(root.newSecretSavedText)
                                                }
                                            },
                                                function () {
                                            }
                                        )
                                    }
                                }
                            }
                        }

                        TextFieldType {
                            id: publicHostTextField
                            Layout.topMargin: 12
                            enabled: fieldsEditable
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            headerText: qsTr("Public host / IP")
                            textField.placeholderText: ServersUiController.serverHostName(ServersUiController.processedServerId)
                            textField.maximumLength: 253
                            textField.validator: PublicHostInputValidator {
                            }
                            textField.onTextChanged: {
                                var t = publicHostTextField.textField.text
                                if (root.configModel.isPublicHostTypingIncomplete(t)) {
                                    publicHostTextField.errorText = ""
                                } else if (!root.configModel.isValidPublicHost(t)) {
                                    publicHostTextField.errorText = qsTr("Enter a valid IP address or domain name")
                                } else {
                                    publicHostTextField.errorText = ""
                                }
                            }
                            textField.onEditingFinished: {
                                textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                                if (!root.configModel.isValidPublicHost(textField.text)) {
                                    publicHostTextField.errorText = qsTr("Enter a valid IP address or domain name")
                                    return
                                }
                                publicHostTextField.errorText = ""
                                if (textField.text !== root.configModel.publicHost) {
                                    root.configModel.publicHost = textField.text
                                }
                            }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            visible: publicHostTextField.textField.text === ""
                            text: qsTr("Leave empty to use server IP automatically")
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            visible: publicHostTextField.textField.text !== "" &&
                                publicHostTextField.textField.text !== ServersUiController.serverHostName(ServersUiController.processedServerId)
                            text: qsTr("⚠ This overrides the server IP in connection links. Make sure this host/domain points to your server.")
                            color: AmneziaStyle.color.textAccent
                        }

                        TextFieldType {
                            id: portTextField
                            enabled: fieldsEditable
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            headerText: qsTr("Server port")
                            textField.placeholderText: root.configModel.defaultPort()
                            textField.maximumLength: 5
                            textField.inputMethodHints: Qt.ImhDigitsOnly
                            textField.validator: IntValidator {
                                bottom: 1
                                top: 65535
                            }
                            textField.onTextChanged: {
                                var cur = portTextField.textField.text
                                var clean = root.configModel.sanitizePortFieldText(cur)
                                if (clean !== cur) {
                                    textField.text = clean
                                    textField.cursorPosition = clean.length
                                }
                            }
                            textField.onEditingFinished: {
                                textField.text = root.configModel.sanitizePortFieldText(textField.text)
                            }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            visible: root.configModel.transportMode === "faketls" && portTextField.textField.text !== "443" && portTextField.textField.text !== ""
                            text: qsTr("FakeTLS may not work on ports other than 443")
                            color: AmneziaStyle.color.textAccent
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            text: qsTr("The promoted channel is set in @MTProxyBot. Paste the proxy tag here: exactly 32 hexadecimal characters (0-9, A-F), as in the bot message — or leave empty.")
                        }

                        TextFieldType {
                            id: tagTextField
                            enabled: fieldsEditable
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            headerText: qsTr("Promoted channel tag (optional)")
                            textField.placeholderText: qsTr("32 hex chars from @MTProxyBot (e.g. 3b7b2fa9…)")
                            textField.maximumLength: root.configModel.mtProxyBotTagHexLength()
                            textField.onTextChanged: {
                                var cur = tagTextField.textField.text
                                var clean = root.configModel.sanitizeMtProxyTagFieldText(cur)
                                if (clean !== cur) {
                                    textField.text = clean
                                    textField.cursorPosition = clean.length
                                    return
                                }
                                var tt = tagTextField.textField.text
                                if (tt === "") {
                                    tagTextField.errorText = ""
                                    return
                                }
                                if (root.configModel.isMtProxyTagTypingIncomplete(tt)) {
                                    tagTextField.errorText = ""
                                    return
                                }
                                if (!root.configModel.isValidMtProxyTag(tt)) {
                                    tagTextField.errorText = qsTr("Proxy tag must be exactly 32 hexadecimal characters (0-9, A-F).")
                                    return
                                }
                                tagTextField.errorText = ""
                            }
                            textField.onEditingFinished: {
                                var raw = textField.text.replace(/^\s+|\s+$/g, '')
                                var normalized = root.configModel.sanitizeMtProxyTagFieldText(raw)
                                textField.text = normalized
                                if (!root.configModel.isValidMtProxyTag(normalized)) {
                                    tagTextField.errorText = qsTr("Proxy tag must be exactly 32 hexadecimal characters (0-9, A-F). Leave empty if unused.")
                                    return
                                }
                                tagTextField.errorText = ""
                                if (normalized !== root.configModel.tag) {
                                    root.configModel.tag = normalized
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            spacing: 4

                            CaptionTextType {
                                text: qsTr("Get a tag from")
                            }
                            CaptionTextType {
                                text: "@MTProxyBot"
                                color: AmneziaStyle.color.textAccent
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Qt.openUrlExternally("https://telegram.me/MTProxyBot")
                                }
                            }
                        }

                        DropDownType {
                            id: transportModeDropDown
                            Layout.topMargin: 12
                            enabled: fieldsEditable
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            drawerHeight: 0.4
                            descriptionText: qsTr("Transport mode")
                            headerText: qsTr("Transport mode")
                            text: root.configModel.transportMode === "faketls" ? qsTr("FakeTLS") : qsTr("Standard MTProto")

                            listView: Component {
                                ListViewType {
                                    leftMargin: 16
                                    rightMargin: 16

                                    model: [qsTr("Standard MTProto"), qsTr("FakeTLS")]
                                    delegate: ListItemType {
                                        width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
                                        text: modelData
                                        trailingIconSource: {
                                            var isCurrent = (index === 0 && root.configModel.transportMode === "standard") ||
                                                (index === 1 && root.configModel.transportMode === "faketls")
                                            return isCurrent ? "qrc:/images/controls/check.svg" : ""
                                        }
                                        onClicked: {
                                            root.configModel.transportMode = (index === 0) ? "standard" : "faketls"
                                            root.syncedSecretTabIndex = root.configModel.transportMode === "faketls" ? 1 : 0
                                            transportModeDropDown.close()
                                        }
                                    }
                                }
                            }
                        }

                        TextFieldType {
                            id: tlsDomainTextField
                            enabled: fieldsEditable
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            visible: root.configModel.transportMode === "faketls"
                            headerText: qsTr("FakeTLS domain")
                            textField.placeholderText: root.previousTlsDomain
                            textField.validator: RegularExpressionValidator {
                                regularExpression: /^[A-Za-z0-9.-]*$/
                            }
                            textField.onTextChanged: {
                                var t = tlsDomainTextField.textField.text
                                if (t === "" || root.configModel.isFakeTlsDomainTypingIncomplete(t)
                                    || root.configModel.isValidFakeTlsDomain(t)) {
                                    tlsDomainTextField.errorText = ""
                                } else {
                                    tlsDomainTextField.errorText = qsTr("Enter a valid domain name")
                                }
                            }
                            textField.onEditingFinished: {
                                textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                                var domainValue = textField.text === "" ? root.configModel.defaultTlsDomain() : textField.text
                                if (!root.configModel.isValidFakeTlsDomain(domainValue)) {
                                    tlsDomainTextField.errorText = qsTr("Enter a valid domain name")
                                    return
                                }
                                tlsDomainTextField.errorText = ""
                                if (domainValue !== root.configModel.tlsDomain) {
                                    root.configModel.tlsDomain = domainValue
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            spacing: 4
                            visible: root.configModel.transportMode === "faketls"

                            CaptionTextType {
                                Layout.fillWidth: true
                                text: qsTr("The domain is encoded into the FakeTLS client secret (ee + base_secret + hex(domain)). It must support HTTPS / TLS 1.3.")
                            }
                            CaptionTextType {
                                Layout.fillWidth: true
                                text: qsTr("\u26a0 Changing the domain will invalidate all previously issued FakeTLS connection links.")
                                color: AmneziaStyle.color.textAccent
                            }
                        }

                        ListItemType {
                            id: advancedHeader
                            Layout.topMargin: 12
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            property bool expanded: false
                            text: qsTr("Advanced")
                            trailingIconSource: expanded
                                ? "qrc:/images/controls/chevron-up.svg"
                                : "qrc:/images/controls/chevron-down.svg"
                            onClicked: {
                                expanded = !expanded
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            visible: advancedHeader.expanded
                            enabled: fieldsEditable

                            SmallTextType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                text: qsTr("Additional secrets")
                            }
                            CaptionTextType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                text: qsTr("Add extra secrets to allow gradual migration without disconnecting existing users.")
                                color: AmneziaStyle.color.textQuaternary
                            }

                            Repeater {
                                model: root.configModel.additionalSecrets
                                delegate: ColumnLayout {
                                    id: addSecretDelegate
                                    property bool linksExpanded: false
                                    readonly property bool linksPanelAllowed: root.isPersistedAdditionalHex(modelData)
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 16
                                    Layout.rightMargin: 16
                                    spacing: 0

                                    onLinksPanelAllowedChanged: {
                                        if (!linksPanelAllowed) {
                                            linksExpanded = false
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: collapsedBar.implicitHeight + 16
                                        color: AmneziaStyle.color.bgSurface
                                        radius: 16

                                        RowLayout {
                                            id: collapsedBar
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.leftMargin: 12
                                            anchors.rightMargin: 8
                                            spacing: 8

                                            Item {
                                                Layout.fillWidth: true
                                                implicitHeight: Math.max(hexCaption.implicitHeight, 24)

                                                RowLayout {
                                                    anchors.left: parent.left
                                                    anchors.right: parent.right
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    spacing: 8

                                                    SmallTextType {
                                                        id: hexCaption
                                                        Layout.fillWidth: true
                                                        text: root.activeSecretForBaseHex(modelData)
                                                        color: AmneziaStyle.color.textSecondary
                                                        elide: Text.ElideMiddle
                                                    }

                                                    Image {
                                                        width: 24
                                                        height: 24
                                                        visible: addSecretDelegate.linksPanelAllowed
                                                        source: "qrc:/images/controls/chevron-down.svg"
                                                        sourceSize.width: 24
                                                        sourceSize.height: 24
                                                        rotation: addSecretDelegate.linksExpanded ? 180 : 0
                                                        Behavior on rotation {
                                                            NumberAnimation {
                                                                duration: 150
                                                            }
                                                        }
                                                    }
                                                }

                                                MouseArea {
                                                    anchors.fill: parent
                                                    visible: addSecretDelegate.linksPanelAllowed
                                                    enabled: addSecretDelegate.linksPanelAllowed
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: addSecretDelegate.linksExpanded = !addSecretDelegate.linksExpanded
                                                }
                                            }

                                            IconButtonType {
                                                size: IconButtonType.Small

                                                icon.source: "qrc:/images/controls/trash.svg"
                                                icon.color: AmneziaStyle.color.statusDestructive
                                                onClicked: root.configModel.removeAdditionalSecret(index)
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        visible: addSecretDelegate.linksPanelAllowed && addSecretDelegate.linksExpanded

                                        SmallTextType {
                                            Layout.fillWidth: true
                                            text: qsTr("Use Telegram connection link")
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: expTmeRow.implicitHeight + 16
                                            color: AmneziaStyle.color.bgSurface
                                            radius: 16

                                            RowLayout {
                                                id: expTmeRow
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                anchors.verticalCenter: parent.verticalCenter
                                                anchors.leftMargin: 12
                                                anchors.rightMargin: 8
                                                spacing: 4

                                                SmallTextType {
                                                    Layout.fillWidth: true
                                                    text: root.tmeLinkFor(modelData)
                                                    color: AmneziaStyle.color.textAccent
                                                    elide: Text.ElideRight
                                                    maximumLineCount: 1
                                                }

                                                IconButtonType {
                                                    size: IconButtonType.Small

                                                    icon.source: "qrc:/images/controls/copy.svg"
                                                    onClicked: root.copyText(root.tmeLinkFor(modelData))
                                                }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: expTgRow.implicitHeight + 16
                                            color: AmneziaStyle.color.bgSurface
                                            radius: 16

                                            RowLayout {
                                                id: expTgRow
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                anchors.verticalCenter: parent.verticalCenter
                                                anchors.leftMargin: 12
                                                anchors.rightMargin: 8
                                                spacing: 4

                                                SmallTextType {
                                                    Layout.fillWidth: true
                                                    text: root.tgLinkFor(modelData)
                                                    color: AmneziaStyle.color.textAccent
                                                    elide: Text.ElideRight
                                                    maximumLineCount: 1
                                                }

                                                IconButtonType {
                                                    size: IconButtonType.Small

                                                    icon.source: "qrc:/images/controls/qr-code.svg"
                                                    onClicked: root.shareQr(root.tgLinkFor(modelData))
                                                }

                                                IconButtonType {
                                                    size: IconButtonType.Small

                                                    icon.source: "qrc:/images/controls/copy.svg"
                                                    onClicked: root.copyText(root.tgLinkFor(modelData))
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            ButtonType {
                                Layout.fillWidth: true

                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                variant: ButtonType.Secondary
                                text: qsTr("Add additional secret")
                                onClicked: {
                                    root.configModel.addAdditionalSecret()
                                }
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                Layout.topMargin: 12
                                Layout.leftMargin: 16
                                visible: !root.isTelemt
                                text: qsTr("Worker mode")
                            }

                            SegmentedControlType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                visible: !root.isTelemt && root.configModel.transportMode !== "faketls"

                                model: [qsTr("Auto"), qsTr("Manual")]
                                currentIndex: root.configModel.workersMode === "manual" ? 1 : 0

                                onActivated: function(index) {
                                    root.configModel.workersMode = index === 1 ? "manual" : "auto"
                                }
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                visible: !root.isTelemt && root.configModel.transportMode === "faketls"
                                text: qsTr("Workers are set to 0 automatically for FakeTLS mode.")
                            }

                            TextFieldType {
                                id: workersTextField
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                visible: !root.isTelemt && root.configModel.workersMode === "manual" && root.configModel.transportMode !== "faketls"
                                headerText: qsTr("Workers count")
                                textField.placeholderText: "2"
                                textField.maximumLength: 2
                                textField.inputMethodHints: Qt.ImhDigitsOnly
                                textField.validator: IntValidator {
                                    bottom: 0
                                    top: root.configModel.maxWorkers()
                                }
                                textField.onTextChanged: {
                                    var cur = workersTextField.textField.text
                                    if (cur === "") {
                                        return
                                    }
                                    var n = parseInt(cur, 10)
                                    var maxW = root.configModel.maxWorkers()
                                    if (isNaN(n) || n < 0) {
                                        n = 0
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
                                textField.onTextEdited: root.configModel.workers = workersTextField.textField.text
                                textField.onEditingFinished: {
                                    var v = workersTextField.textField.text
                                    if (v !== "") {
                                        var m = parseInt(v, 10)
                                        var maxW2 = root.configModel.maxWorkers()
                                        if (isNaN(m) || m < 0) {
                                            m = 0
                                        }
                                        if (m > maxW2) {
                                            m = maxW2
                                        }
                                        v = String(m)
                                        textField.text = v
                                    }
                                    if (v !== root.configModel.workers) {
                                        root.configModel.workers = v
                                    }
                                }
                            }

                            SwitchRowType {
                                Layout.fillWidth: true
                                Layout.topMargin: 12
                                Layout.rightMargin: 16
                                Layout.leftMargin: 16
                                text: root.natSwitchText
                                descriptionText: root.natSwitchDescriptionText
                                checked: root.configModel.natEnabled
                                onToggled: function () {
                                    if (checked !== root.configModel.natEnabled) {
                                        root.configModel.natEnabled = checked
                                    }
                                }
                            }

                            TextFieldType {
                                id: natInternalIpTextField
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                visible: !root.isTelemt && root.configModel.natEnabled
                                headerText: qsTr("Internal IP")
                                textField.placeholderText: "172.17.0.2"
                                textField.maximumLength: 15
                                textField.validator: RegularExpressionValidator {
                                    regularExpression: root.natIpv4InputFormat
                                }
                                textField.onTextChanged: {
                                    if (root.natIpv4FieldShowInvalidError(textField.text)) {
                                        natInternalIpTextField.errorText = qsTr("Enter a valid IPv4 address")
                                    } else {
                                        natInternalIpTextField.errorText = ""
                                    }
                                }
                                textField.onEditingFinished: {
                                    textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                                    if (!root.configModel.isValidOptionalIpv4(textField.text)) {
                                        natInternalIpTextField.errorText = qsTr("Enter a valid IPv4 address")
                                        return
                                    }
                                    natInternalIpTextField.errorText = ""
                                    if (textField.text !== root.configModel.natInternalIp) {
                                        root.configModel.natInternalIp = textField.text
                                    }
                                }
                            }

                            TextFieldType {
                                id: natExternalIpTextField
                                Layout.fillWidth: true
                                Layout.leftMargin: 16
                                Layout.rightMargin: 16
                                visible: root.configModel.natEnabled
                                headerText: root.natExternalIpHeaderText
                                textField.placeholderText: "1.2.3.4"
                                textField.maximumLength: 15
                                textField.validator: RegularExpressionValidator {
                                    regularExpression: root.natIpv4InputFormat
                                }
                                textField.onTextChanged: {
                                    if (root.natIpv4FieldShowInvalidError(textField.text)) {
                                        natExternalIpTextField.errorText = qsTr("Enter a valid IPv4 address")
                                    } else {
                                        natExternalIpTextField.errorText = ""
                                    }
                                }
                                textField.onEditingFinished: {
                                    textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                                    if (!root.configModel.isValidOptionalIpv4(textField.text)) {
                                        natExternalIpTextField.errorText = qsTr("Enter a valid IPv4 address")
                                        return
                                    }
                                    natExternalIpTextField.errorText = ""
                                    if (textField.text !== root.configModel.natExternalIp) {
                                        root.configModel.natExternalIp = textField.text
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.topMargin: 12
                            spacing: 8
                            visible: containerStatus === 1

                            RowLayout {
                                Layout.fillWidth: true

                                SectionHeaderType {
                                    Layout.fillWidth: true
                                    level: SectionHeaderType.Section
                                    headerText: qsTr("Diagnostics")
                                }

                                IconButtonType {
                                    size: IconButtonType.Small

                                    icon.source: "qrc:/images/controls/refresh-cw.svg"
                                    icon.color: diagLoading ? AmneziaStyle.color.iconSecondary : AmneziaStyle.color.bgSurfaceInverse
                                    hoverEnabled: !diagLoading
                                    enabled: !diagLoading
                                    onClicked: {
                                        diagLoading = true
                                        InstallController.refreshContainerDiagnostics(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, parseInt(root.configModel.port))
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: diagClientsConnected >= 0 ? (diagPortReachable ? AmneziaStyle.color.bgSurfaceInverse : AmneziaStyle.color.statusDestructive) : AmneziaStyle.color.bgActionPress
                                }
                                SmallTextType {
                                    Layout.fillWidth: true
                                    text: qsTr("Public port reachable")
                                    color: AmneziaStyle.color.textSecondary
                                }
                                SmallTextType {
                                    text: diagClientsConnected < 0 ? qsTr("—") : (diagPortReachable ? qsTr("Yes") : qsTr("No"))
                                    color: diagClientsConnected >= 0 ? (diagPortReachable ? AmneziaStyle.color.textSecondary : AmneziaStyle.color.statusDestructive) : AmneziaStyle.color.textTertiary
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: diagClientsConnected >= 0 ? (diagTelegramReachable ? AmneziaStyle.color.bgSurfaceInverse : AmneziaStyle.color.statusDestructive) : AmneziaStyle.color.bgActionPress
                                }
                                SmallTextType {
                                    Layout.fillWidth: true
                                    text: qsTr("Telegram upstream reachable")
                                    color: AmneziaStyle.color.textSecondary
                                }
                                SmallTextType {
                                    text: diagClientsConnected < 0 ? qsTr("—") : (diagTelegramReachable ? qsTr("Yes") : qsTr("No"))
                                    color: diagClientsConnected >= 0 ? (diagTelegramReachable ? AmneziaStyle.color.textSecondary : AmneziaStyle.color.statusDestructive) : AmneziaStyle.color.textTertiary
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: diagClientsConnected >= 0 ? AmneziaStyle.color.accentLight : AmneziaStyle.color.bgActionPress
                                }
                                SmallTextType {
                                    Layout.fillWidth: true
                                    text: qsTr("Clients connected")
                                    color: AmneziaStyle.color.textSecondary
                                }
                                SmallTextType {
                                    text: diagClientsConnected < 0 ? qsTr("—") : diagClientsConnected.toString()
                                    color: AmneziaStyle.color.textSecondary
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: AmneziaStyle.color.bgActionPress
                                }
                                SmallTextType {
                                    Layout.fillWidth: true
                                    text: qsTr("Last config refresh")
                                    color: AmneziaStyle.color.textSecondary
                                }
                                SmallTextType {
                                    text: diagLastConfigRefresh !== "" ? diagLastConfigRefresh : qsTr("—")
                                }
                            }

                            ListItemType {
                                Layout.fillWidth: true
                                Layout.leftMargin: -16
                                visible: diagStatsEndpoint !== ""
                                overline: qsTr("Stats endpoint")
                                text: diagStatsEndpoint
                                trailingIconSource: "qrc:/images/controls/copy.svg"
                                onClicked: {
                                    GC.copyToClipBoard(diagStatsEndpoint)
                                    PageController.showNotificationMessage(qsTr("Copied"))
                                }
                            }

                            SmallTextType {
                                Layout.fillWidth: true
                                text: diagLoading ? qsTr("Refreshing…") : qsTr("Tap ↻ to refresh diagnostics")
                                visible: diagClientsConnected < 0
                            }
                        }

                        ButtonType {
                            Layout.fillWidth: true
                            Layout.topMargin: 12
                            Layout.rightMargin: 16
                            Layout.leftMargin: 16
                            visible: ServersUiController.isProcessedServerHasWriteAccess()
                            enabled: fieldsEditable && !root.networkBlocked
                            text: qsTr("Save")
                            onClicked: {
                                if (root.networkBlocked) {
                                    PageController.showErrorMessage(root.noInternetText)
                                    return
                                }
                                publicHostTextField.errorText = ""
                                tagTextField.errorText = ""
                                tlsDomainTextField.errorText = ""
                                natInternalIpTextField.errorText = ""
                                natExternalIpTextField.errorText = ""
                                portTextField.errorText = ""

                                var portValue = portTextField.textField.text === ""
                                    ? root.configModel.defaultPort()
                                    : portTextField.textField.text

                                var errorLines = []
                                var bullet = "- "
                                if (!portTextField.textField.acceptableInput && portTextField.textField.text !== "") {
                                    var portErr = qsTr("The port must be in the range of 1 to 65535")
                                    portTextField.errorText = portErr
                                    errorLines.push(bullet + portErr)
                                }
                                if (!root.configModel.isValidPublicHost(publicHostTextField.textField.text)) {
                                    var hostErr = qsTr("Enter a valid IP address or domain name")
                                    publicHostTextField.errorText = hostErr
                                    errorLines.push(bullet + hostErr)
                                }
                                var tagNormalized = root.configModel.sanitizeMtProxyTagFieldText(tagTextField.textField.text)
                                tagTextField.textField.text = tagNormalized
                                if (!root.configModel.isValidMtProxyTag(tagNormalized)) {
                                    var tagErr = qsTr("Proxy tag must be exactly 32 hexadecimal characters (0-9, A-F), or leave empty.")
                                    tagTextField.errorText = tagErr
                                    errorLines.push(bullet + tagErr)
                                }
                                var domainValueForSave = tlsDomainTextField.textField.text === ""
                                    ? root.configModel.defaultTlsDomain()
                                    : tlsDomainTextField.textField.text
                                if (!root.configModel.isValidFakeTlsDomain(domainValueForSave)) {
                                    var tlsErr = qsTr("Enter a valid domain name")
                                    tlsDomainTextField.errorText = tlsErr
                                    errorLines.push(bullet + tlsErr)
                                }
                                var natIpErr = qsTr("Enter a valid IPv4 address")
                                if (!root.isTelemt && !root.configModel.isValidOptionalIpv4(natInternalIpTextField.textField.text)) {
                                    natInternalIpTextField.errorText = natIpErr
                                    errorLines.push(bullet + qsTr("NAT internal IP: enter a valid IPv4 address"))
                                }
                                if (!root.configModel.isValidOptionalIpv4(natExternalIpTextField.textField.text)) {
                                    natExternalIpTextField.errorText = natIpErr
                                    errorLines.push(bullet + root.natExternalIpErrorText)
                                }
                                if (errorLines.length > 0) {
                                    PageController.showErrorMessage(errorLines.join("\n"))
                                    return
                                }
                                root.configModel.setPort(portValue)
                                root.configModel.setTag(tagNormalized)
                                root.configModel.setPublicHost(publicHostTextField.textField.text)
                                root.configModel.setTransportMode(root.configModel.transportMode)
                                var domainValue = tlsDomainTextField.textField.text === ""
                                    ? root.configModel.defaultTlsDomain()
                                    : tlsDomainTextField.textField.text
                                root.configModel.setTlsDomain(domainValue)

                                if (!root.isTelemt) {
                                    if (root.configModel.transportMode === "faketls") {
                                        root.configModel.workers = "0"
                                    } else {
                                        root.configModel.setWorkersMode(root.configModel.workersMode)
                                        root.configModel.setWorkers(root.configModel.workers)
                                    }
                                }
                                root.configModel.setNatEnabled(root.configModel.natEnabled)
                                if (!root.isTelemt) {
                                    root.configModel.setNatInternalIp(natInternalIpTextField.textField.text)
                                }
                                root.configModel.setNatExternalIp(natExternalIpTextField.textField.text)

                                previousPort = root.configModel.port
                                previousTag = root.configModel.tag
                                previousPublicHost = root.configModel.publicHost
                                previousTransportMode = root.configModel.transportMode
                                previousTlsDomain = root.configModel.tlsDomain
                                previousWorkersMode = root.configModel.workersMode
                                previousWorkers = root.configModel.workers
                                previousNatEnabled = root.configModel.natEnabled
                                previousNatInternalIp = root.configModel.natInternalIp
                                previousNatExternalIp = root.configModel.natExternalIp
                                root.previousSecret = root.configModel.secret
                                isUpdating = true
                                root.scheduleUpdate(false)
                            }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("If you change the settings, the proxy connection link will change. The old link will stop working.")
                        }
                    }
                }
            }
        }
    }
}
