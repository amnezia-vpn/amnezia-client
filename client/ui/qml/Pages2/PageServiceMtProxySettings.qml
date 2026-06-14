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

    property int containerStatus: 1
    property bool isUpdating: false
    property bool isCheckingStatus: false
    property bool isFetchingSecret: false
    property bool previousEnabled: true
    property int previousContainerStatus: 1

    property string previousPort: ""
    property string previousTag: ""
    property string previousPublicHost: ""
    property string previousTransportMode: MtProxyConfigModel.transportModeStandard()
    property string previousTlsDomain: MtProxyConfigModel.defaultTlsDomain()
    property string previousWorkersMode: MtProxyConfigModel.workersModeAuto()
    property string previousWorkers: MtProxyConfigModel.defaultWorkers()
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

    readonly property bool mtProxyNetworkBlocked: !NetworkReachabilityController.hasInternetAccess

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

    function mtProxyDomainToHex(domain) {
        var hex = ""
        for (var i = 0; i < domain.length; i++) {
            var code = domain.charCodeAt(i).toString(16)
            hex += (code.length < 2 ? "0" : "") + code
        }
        return hex
    }

    function mtProxyClientSecret(baseHex32, mode, tlsDomain) {
        if (baseHex32 === "") {
            return ""
        }
        if (mode === "faketls") {
            return "ee" + baseHex32 + mtProxyDomainToHex(tlsDomain)
        }
        return "dd" + baseHex32
    }

    function mtProxyClientSecretForTabIndex(baseHex32, tabIndex, tlsDomain, defaultTlsDomain) {
        var domain = tlsDomain !== "" ? tlsDomain : defaultTlsDomain
        if (tabIndex === 1) {
            return mtProxyClientSecret(baseHex32, "faketls", domain)
        }
        return mtProxyClientSecret(baseHex32, "standard", domain)
    }

    property bool containerStatusRefreshCallPending: false

    function mtProxyRequestContainerStatusRefresh() {
        if (!NetworkReachabilityController.hasInternetAccess) {
            isCheckingStatus = false
            syncPageBusyIndicator()
            return
        }
        isCheckingStatus = true
        syncPageBusyIndicator()
        InstallController.refreshContainerStatus(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
    }

    function mtProxyScheduleContainerStatusRefresh() {
        if (containerStatusRefreshCallPending) {
            return
        }
        containerStatusRefreshCallPending = true
        Qt.callLater(function () {
            containerStatusRefreshCallPending = false
            root.mtProxyRequestContainerStatusRefresh()
        })
    }

    function mtProxyOnPageShown() {
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
        root.mtProxyScheduleContainerStatusRefresh()
    }

    property var mtProxyPersistedAdditionalHex: []

    function mtProxyRefreshPersistedAdditionalSecrets() {
        var list = MtProxyConfigModel.additionalSecretsList()
        var a = []
        for (var i = 0; i < list.length; ++i) {
            a.push(String(list[i]))
        }
        root.mtProxyPersistedAdditionalHex = a
    }

    function mtProxyIsPersistedAdditionalHex(hex) {
        var h = String(hex)
        for (var j = 0; j < root.mtProxyPersistedAdditionalHex.length; ++j) {
            if (String(root.mtProxyPersistedAdditionalHex[j]) === h) {
                return true
            }
        }
        return false
    }

    readonly property var natIpv4InputFormat: /^(\d{1,3}\.){0,3}\d{0,3}$/

    function mtProxyScheduleUpdate(closePage) {
        var cp = closePage === undefined ? false : closePage
        Qt.callLater(function () {
            InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.MtProxy, cp)
        })
    }

    function natIpv4FieldShowInvalidError(text) {
        var t = text ? String(text).replace(/^\s+|\s+$/g, '') : ""
        if (t === "")
            return false
        if (MtProxyConfigModel.isValidOptionalIpv4(t))
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
        root.savedTransportMode = MtProxyConfigModel.getTransportMode()
        root.savedTlsDomain = MtProxyConfigModel.getTlsDomain()
        root.savedPublicHost = MtProxyConfigModel.getPublicHost()

        Qt.callLater(function () {
            root.mtProxyRefreshPersistedAdditionalSecrets()
        })

        Qt.callLater(root.mtProxyOnPageShown)
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
            PageController.disableControls(false)
            PageController.showBusyIndicator(false)
            diagLoading = false
        } else {
            root.mtProxyOnPageShown()
        }
    }

    Connections {
        target: NetworkReachabilityController

        function onHasInternetAccessChanged() {
            if (!root.visible) {
                return
            }
            if (NetworkReachabilityController.hasInternetAccess) {
                root.mtProxyScheduleContainerStatusRefresh()
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
            root.savedTransportMode = MtProxyConfigModel.getTransportMode()
            root.savedTlsDomain = MtProxyConfigModel.getTlsDomain()
            root.savedPublicHost = MtProxyConfigModel.getPublicHost()
            root.mtProxyRefreshPersistedAdditionalSecrets()
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
            MtProxyConfigModel.setEnabled(previousEnabled)
            MtProxyConfigModel.setPort(previousPort)
            MtProxyConfigModel.setTag(previousTag)
            MtProxyConfigModel.setPublicHost(previousPublicHost)
            MtProxyConfigModel.setTransportMode(previousTransportMode)
            MtProxyConfigModel.setTlsDomain(previousTlsDomain)
            MtProxyConfigModel.setWorkersMode(previousWorkersMode)
            MtProxyConfigModel.setWorkers(previousWorkers)
            MtProxyConfigModel.setNatEnabled(previousNatEnabled)
            MtProxyConfigModel.setNatInternalIp(previousNatInternalIp)
            MtProxyConfigModel.setNatExternalIp(previousNatExternalIp)
            if (previousSecret !== "") {
                MtProxyConfigModel.setSecret(previousSecret)
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
                root.mtProxyScheduleUpdate(false)
                return
            }
            isUpdating = false
            containerStatus = enabled ? 1 : 2
            PageController.showNotificationMessage(
                enabled ? qsTr("MTProxy started") : qsTr("MTProxy stopped"))
        }

        function onContainerStatusRefreshed(status) {
            if (!root.visible) {
                isCheckingStatus = false
                isFetchingSecret = false
                return
            }
            containerStatus = status

            root.savedTransportMode = MtProxyConfigModel.getTransportMode()
            root.savedTlsDomain = MtProxyConfigModel.getTlsDomain()
            root.savedPublicHost = MtProxyConfigModel.getPublicHost()
            if (status === 1) {
                MtProxyConfigModel.setEnabled(true)
                isFetchingSecret = true
                isCheckingStatus = false
                InstallController.fetchContainerSecret(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
            } else {
                isFetchingSecret = false
                isCheckingStatus = false
                if (status === 2) {
                    MtProxyConfigModel.setEnabled(false)
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
            MtProxyConfigModel.validateAndSetSecret(secret)
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

            headerText: qsTr("MTProxy settings")
            descriptionLinkText: qsTr("Read more about this settings")
            descriptionLinkUrl: "https://core.telegram.org/proxy"
        }

        CaptionTextType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 8
            visible: root.mtProxyNetworkBlocked
            text: qsTr("No internet connection. Connect to the internet to change MTProxy settings.")
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
        id: tabContent
        anchors.top: mainTabBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: mainTabBar.currentIndex

        ListViewType {
            id: connectionListView
            model: MtProxyConfigModel

            delegate: ColumnLayout {
                width: connectionListView.width
                spacing: 0

                property int secretTabIndex: root.syncedSecretTabIndex

                function activeSecret() {
                    return root.mtProxyClientSecretForTabIndex(secret, root.syncedSecretTabIndex,
                        root.savedTlsDomain, MtProxyConfigModel.defaultTlsDomain())
                }

                function effectiveSecret() {
                    return activeSecret()
                }

                function effectiveHost() {
                    return root.savedPublicHost !== "" ? root.savedPublicHost : ServersUiController.serverHostName(ServersUiController.processedServerId)
                }

                function tmeLink() {
                    return "https://t.me/proxy?server=" + effectiveHost() + "&port=" + port + "&secret=" + activeSecret()
                }

                function tgLink() {
                    return "tg://proxy?server=" + effectiveHost() + "&port=" + port + "&secret=" + activeSecret()
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("Use Telegram connection link")
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
                            text: secret !== "" ? tmeLink() : qsTr("Deploy MTProxy first")
                            color: secret !== "" ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.mutedGray
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
                            visible: secret !== ""
                            onClicked: {
                                ExportController.generateQrFromString(tmeLink())
                                PageController.goToShareConnectionPage(
                                    qsTr("Telegram connection link"),
                                    qsTr("MTProxy connection link"),
                                    "", "", "")
                            }
                        }

                        ImageButtonType {
                            implicitWidth: 36
                            implicitHeight: 36
                            hoverEnabled: true
                            image: "qrc:/images/controls/copy.svg"
                            imageColor: AmneziaStyle.color.paleGray
                            visible: secret !== ""
                            onClicked: {
                                GC.copyToClipBoard(tmeLink())
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
                            text: tgLink()
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
                                ExportController.generateQrFromString(tgLink())
                                PageController.goToShareConnectionPage(
                                    qsTr("Telegram connection link"),
                                    qsTr("MTProxy connection link"),
                                    "", "", "")
                            }
                        }

                        ImageButtonType {
                            implicitWidth: 36
                            implicitHeight: 36
                            hoverEnabled: true
                            image: "qrc:/images/controls/copy.svg"
                            imageColor: AmneziaStyle.color.paleGray
                            onClicked: {
                                GC.copyToClipBoard(tgLink())
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
                            onClicked: Qt.openUrlExternally("https://core.telegram.org/proxy")
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
                    Layout.bottomMargin: 32
                    implicitHeight: manualCol.implicitHeight + 8
                    color: AmneziaStyle.color.onyxBlack
                    radius: 8
                    border.color: AmneziaStyle.color.slateGray
                    border.width: 1

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
                                onClicked: { GC.copyToClipBoard(effectiveHost())
                                    PageController.showNotificationMessage(qsTr("Copied")) }
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
                                    text: qsTr("Port")
                                    color: AmneziaStyle.color.mutedGray
                                    font.pixelSize: 12
                                }
                                CaptionTextType {
                                    Layout.fillWidth: true
                                    text: port
                                    color: AmneziaStyle.color.paleGray
                                }
                            }
                            ImageButtonType {
                                implicitWidth: 36
                                implicitHeight: 36
                                hoverEnabled: true
                                image: "qrc:/images/controls/copy.svg"
                                imageColor: AmneziaStyle.color.paleGray
                                onClicked: { GC.copyToClipBoard(port)
                                    PageController.showNotificationMessage(qsTr("Copied")) }
                            }
                        }

                        DividerType {
                            Layout.fillWidth: true
                        }

                        ButtonGroup {
                            id: secretTabGroup
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 8
                            Layout.topMargin: 4
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
                                    text: activeSecret()
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
                                onClicked: { GC.copyToClipBoard(activeSecret())
                                    PageController.showNotificationMessage(qsTr("Copied")) }
                            }
                        }
                    }
                }

                LabelWithButtonType {
                    id: removeButton
                    Layout.fillWidth: true
                    Layout.bottomMargin: 24
                    Layout.leftMargin: 0
                    Layout.rightMargin: 16
                    visible: ServersUiController.isProcessedServerHasWriteAccess()
                    text: qsTr("Delete MTProxy")
                    textColor: AmneziaStyle.color.vibrantRed
                    clickedFunction: function () {
                        var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                        var descriptionText = qsTr("The proxy will be stopped and all users will lose access.")
                        var yesButtonText = qsTr("Continue")
                        var noButtonText = qsTr("Cancel")
                        var yesButtonFunction = function () {
                            PageController.goToPage(PageEnum.PageDeinstalling)
                            InstallController.removeContainer(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
                        }
                        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, function () {
                        })
                    }
                    MouseArea {
                        anchors.fill: removeButton
                        cursorShape: Qt.PointingHandCursor
                        enabled: false
                    }
                }
            }
        }

        ListViewType {
            id: settingsListView
            model: MtProxyConfigModel
            reuseItems: false

            delegate: ColumnLayout {
                id: settingsRoot
                width: settingsListView.width
                spacing: 0

                function mtProxyActiveSecretForBaseHex(baseHex) {
                    return root.mtProxyClientSecretForTabIndex(baseHex, root.syncedSecretTabIndex,
                        root.savedTlsDomain, MtProxyConfigModel.defaultTlsDomain())
                }

                function mtProxyEffectiveHostForLinks() {
                    return root.savedPublicHost !== "" ? root.savedPublicHost : ServersUiController.serverHostName(ServersUiController.processedServerId)
                }

                function mtProxyTmeLinkForAdditional(baseHex) {
                    return "https://t.me/proxy?server=" + mtProxyEffectiveHostForLinks() + "&port=" + port + "&secret=" + mtProxyActiveSecretForBaseHex(baseHex)
                }

                function mtProxyTgLinkForAdditional(baseHex) {
                    return "tg://proxy?server=" + mtProxyEffectiveHostForLinks() + "&port=" + port + "&secret=" + mtProxyActiveSecretForBaseHex(baseHex)
                }

                SwitcherType {
                    id: enableMtProxySwitch
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    text: qsTr("Enable MTProxy")
                    checked: isEnabled
                    enabled: containerStatus !== 0 && containerStatus !== 3 && !root.pageBusy
                        && !root.mtProxyNetworkBlocked
                    onToggled: function () {
                        if (checked !== isEnabled) {
                            previousEnabled = isEnabled
                            previousContainerStatus = containerStatus
                            root.previousSecret = secret
                            isEnabled = checked
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

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16 * 2
                    spacing: 4

                    CaptionTextType {
                        text: qsTr("Base secret")
                        color: AmneziaStyle.color.mutedGray
                        font.pixelSize: 12
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: secret !== "" ? mtProxyActiveSecretForBaseHex(secret) : qsTr("Not generated")
                            color: secret !== "" ? AmneziaStyle.color.paleGray : AmneziaStyle.color.mutedGray
                            wrapMode: Text.WrapAnywhere
                            font.pixelSize: 14
                        }

                        ImageButtonType {
                            Layout.alignment: Qt.AlignTop
                            implicitWidth: 36
                            implicitHeight: 36
                            hoverEnabled: true
                            image: "qrc:/images/controls/refresh-cw.svg"
                            imageColor: AmneziaStyle.color.paleGray
                            visible: ServersUiController.isProcessedServerHasWriteAccess()
                            onClicked: {
                                var secretSnapshot = secret
                                showQuestionDrawer(
                                    qsTr("Generate new secret?"),
                                    qsTr("All existing connection links will stop working. Users will need new links."),
                                    qsTr("Generate"),
                                    qsTr("Cancel"),
                                        function () {
                                        root.previousSecret = secretSnapshot
                                        if (containerStatus === 1) {
                                            isUpdating = true
                                            MtProxyConfigModel.generateSecret()
                                            root.mtProxyScheduleUpdate(false)
                                        } else {
                                            MtProxyConfigModel.generateSecret()
                                            PageController.showNotificationMessage(qsTr("New secret saved. It will be applied when MTProxy is started."))
                                        }
                                    },
                                        function () {
                                    }
                                )
                            }
                        }
                    }
                }

                TextFieldWithHeaderType {
                    id: publicHostTextField
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 4
                    headerText: qsTr("Public host / IP")
                    textField.placeholderText: ServersUiController.serverHostName(ServersUiController.processedServerId)
                    textField.text: publicHost
                    textField.maximumLength: 253
                    textField.validator: PublicHostInputValidator {
                    }
                    textField.onTextChanged: {
                        var t = publicHostTextField.textField.text
                        if (MtProxyConfigModel.isPublicHostTypingIncomplete(t)) {
                            publicHostTextField.errorText = ""
                        } else if (!MtProxyConfigModel.isValidPublicHost(t)) {
                            publicHostTextField.errorText = qsTr("Enter a valid IP address or domain name")
                        } else {
                            publicHostTextField.errorText = ""
                        }
                    }
                    textField.onEditingFinished: {
                        textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                        if (!MtProxyConfigModel.isValidPublicHost(textField.text)) {
                            publicHostTextField.errorText = qsTr("Enter a valid IP address or domain name")
                            return
                        }
                        publicHostTextField.errorText = ""
                        if (textField.text !== publicHost) {
                            publicHost = textField.text
                            MtProxyConfigModel.setPublicHost(publicHost)
                        }
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 4
                    visible: publicHostTextField.textField.text === ""
                    text: qsTr("Leave empty to use server IP automatically")
                    color: AmneziaStyle.color.mutedGray
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 12
                    visible: publicHostTextField.textField.text !== "" &&
                        publicHostTextField.textField.text !== ServersUiController.serverHostName(ServersUiController.processedServerId)
                    text: qsTr("⚠ This overrides the server IP in connection links. Make sure this host/domain points to your server.")
                    color: AmneziaStyle.color.goldenApricot
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                TextFieldWithHeaderType {
                    id: portTextField
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    headerText: qsTr("Server port")
                    textField.placeholderText: MtProxyConfigModel.defaultPort()
                    textField.maximumLength: 5
                    textField.validator: IntValidator {
                        bottom: 1
                        top: 65535
                    }
                    Component.onCompleted: {
                        var savedPort = port
                        textField.text = (savedPort === MtProxyConfigModel.defaultPort()) ? "" : savedPort
                    }
                    textField.onEditingFinished: {
                        textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 12
                    visible: transportMode === "faketls" && portTextField.textField.text !== "443" && portTextField.textField.text !== ""
                    text: qsTr("FakeTLS may not work on ports other than 443")
                    color: AmneziaStyle.color.goldenApricot
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("The promoted channel is set in @MTProxyBot. Paste the proxy tag here: exactly 32 hexadecimal characters (0-9, A-F), as in the bot message — or leave empty.")
                    color: AmneziaStyle.color.mutedGray
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                TextFieldWithHeaderType {
                    id: tagTextField
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    headerText: qsTr("MTProxy bot tag (optional)")
                    textField.placeholderText: qsTr("32 hex chars from @MTProxyBot (e.g. 3b7b2fa9…)")
                    textField.text: tag
                    textField.maximumLength: MtProxyConfigModel.mtProxyBotTagHexLength()
                    textField.onTextChanged: {
                        var cur = tagTextField.textField.text
                        var clean = MtProxyConfigModel.sanitizeMtProxyTagFieldText(cur)
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
                        if (MtProxyConfigModel.isMtProxyTagTypingIncomplete(tt)) {
                            tagTextField.errorText = ""
                            return
                        }
                        if (!MtProxyConfigModel.isValidMtProxyTag(tt)) {
                            tagTextField.errorText = qsTr("Proxy tag must be exactly 32 hexadecimal characters (0-9, A-F).")
                            return
                        }
                        tagTextField.errorText = ""
                    }
                    textField.onEditingFinished: {
                        var raw = textField.text.replace(/^\s+|\s+$/g, '')
                        var normalized = MtProxyConfigModel.sanitizeMtProxyTagFieldText(raw)
                        textField.text = normalized
                        if (!MtProxyConfigModel.isValidMtProxyTag(normalized)) {
                            tagTextField.errorText = qsTr("Proxy tag must be exactly 32 hexadecimal characters (0-9, A-F). Leave empty if unused.")
                            return
                        }
                        tagTextField.errorText = ""
                        if (normalized !== tag) {
                            tag = normalized
                            MtProxyConfigModel.setTag(tag)
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    spacing: 4

                    CaptionTextType {
                        text: qsTr("Get a tag from")
                        color: AmneziaStyle.color.mutedGray
                        font.pixelSize: 12
                    }
                    CaptionTextType {
                        text: "@MTProxyBot"
                        color: AmneziaStyle.color.goldenApricot
                        font.pixelSize: 12
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.openUrlExternally("https://t.me/MTProxyBot")
                        }
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16 * 2
                    text: qsTr("Transport mode")
                    color: AmneziaStyle.color.mutedGray
                    font.pixelSize: 12
                }

                DropDownType {
                    id: transportModeDropDown
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16

                    drawerParent: root
                    drawerHeight: 0.4
                    headerText: qsTr("Transport mode")
                    text: transportMode === "faketls" ? qsTr("FakeTLS") : qsTr("Standard MTProto")

                    listView: Component {
                        ListViewType {
                            model: [qsTr("Standard MTProto"), qsTr("FakeTLS")]
                            delegate: LabelWithButtonType {
                                Layout.fillWidth: true
                                text: modelData
                                rightImageSource: {
                                    var isCurrent = (index === 0 && transportMode === "standard") ||
                                        (index === 1 && transportMode === "faketls")
                                    return isCurrent ? "qrc:/images/controls/check.svg" : ""
                                }
                                rightImageColor: AmneziaStyle.color.goldenApricot
                                clickedFunction: function () {
                                    transportMode = (index === 0) ? "standard" : "faketls"
                                    MtProxyConfigModel.setTransportMode(transportMode)
                                    root.syncedSecretTabIndex = transportMode === "faketls" ? 1 : 0
                                    transportModeDropDown.closeTriggered()
                                }
                            }
                        }
                    }
                }

                TextFieldWithHeaderType {
                    id: tlsDomainTextField
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    visible: transportMode === "faketls"
                    headerText: qsTr("FakeTLS domain")
                    textField.placeholderText: root.previousTlsDomain
                    Component.onCompleted: {
                        var savedDomain = tlsDomain
                        textField.text = (savedDomain === MtProxyConfigModel.defaultTlsDomain() || savedDomain === "") ? "" : savedDomain
                    }
                    textField.onEditingFinished: {
                        textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                        var domainValue = textField.text === "" ? MtProxyConfigModel.defaultTlsDomain() : textField.text
                        if (!MtProxyConfigModel.isValidFakeTlsDomain(domainValue)) {
                            tlsDomainTextField.errorText = qsTr("Enter a valid domain name")
                            return
                        }
                        tlsDomainTextField.errorText = ""
                        if (domainValue !== tlsDomain) {
                            tlsDomain = domainValue
                            MtProxyConfigModel.setTlsDomain(tlsDomain)
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    spacing: 4
                    visible: transportMode === "faketls"

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("The domain is encoded into the FakeTLS client secret (ee + base_secret + hex(domain)). It must support HTTPS / TLS 1.3.")
                        color: AmneziaStyle.color.mutedGray
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                    }
                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("\u26a0 Changing the domain will invalidate all previously issued FakeTLS connection links.")
                        color: AmneziaStyle.color.goldenApricot
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                    }
                }

                LabelWithButtonType {
                    id: advancedHeader
                    Layout.fillWidth: true
                    Layout.leftMargin: 0
                    Layout.rightMargin: 16
                    property bool expanded: false
                    text: qsTr("Advanced")
                    rightImageSource: expanded
                        ? "qrc:/images/controls/chevron-up.svg"
                        : "qrc:/images/controls/chevron-down.svg"
                    rightImageColor: AmneziaStyle.color.mutedGray
                    clickedFunction: function () {
                        expanded = !expanded
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    visible: advancedHeader.expanded

                    CaptionTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.topMargin: 8
                        Layout.bottomMargin: 4
                        text: qsTr("Additional secrets")
                        color: AmneziaStyle.color.mutedGray
                    }
                    CaptionTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 8
                        text: qsTr("Add extra secrets to allow gradual migration without disconnecting existing users.")
                        color: AmneziaStyle.color.charcoalGray
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                    }

                    Repeater {
                        model: additionalSecrets
                        delegate: ColumnLayout {
                            id: addSecretDelegate
                            property bool linksExpanded: false
                            readonly property bool linksPanelAllowed: root.mtProxyIsPersistedAdditionalHex(modelData)
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.bottomMargin: 8
                            spacing: 0

                            onLinksPanelAllowedChanged: {
                                if (!linksPanelAllowed) {
                                    linksExpanded = false
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: collapsedBar.implicitHeight + 16
                                color: AmneziaStyle.color.onyxBlack
                                radius: 8
                                border.color: AmneziaStyle.color.slateGray
                                border.width: 1

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

                                            CaptionTextType {
                                                id: hexCaption
                                                Layout.fillWidth: true
                                                text: modelData
                                                color: AmneziaStyle.color.paleGray
                                                elide: Text.ElideMiddle
                                                font.pixelSize: 13
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

                                    ImageButtonType {
                                        implicitWidth: 32
                                        implicitHeight: 32
                                        hoverEnabled: true
                                        visible: ServersUiController.isProcessedServerHasWriteAccess()
                                        image: "qrc:/images/controls/trash.svg"
                                        imageColor: AmneziaStyle.color.vibrantRed
                                        onClicked: {
                                            MtProxyConfigModel.removeAdditionalSecret(index)
                                            if (containerStatus === 1) {
                                                root.mtProxyScheduleUpdate(false)
                                            }
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.topMargin: 8
                                spacing: 8
                                visible: addSecretDelegate.linksPanelAllowed && addSecretDelegate.linksExpanded

                                CaptionTextType {
                                    Layout.fillWidth: true
                                    text: qsTr("Use Telegram connection link")
                                    color: AmneziaStyle.color.mutedGray
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: expTmeRow.implicitHeight + 16
                                    color: AmneziaStyle.color.onyxBlack
                                    radius: 8
                                    border.color: AmneziaStyle.color.slateGray
                                    border.width: 1

                                    RowLayout {
                                        id: expTmeRow
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 8
                                        spacing: 4

                                        CaptionTextType {
                                            Layout.fillWidth: true
                                            text: settingsRoot.mtProxyTmeLinkForAdditional(modelData)
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
                                                ExportController.generateQrFromString(settingsRoot.mtProxyTmeLinkForAdditional(modelData))
                                                PageController.goToShareConnectionPage(
                                                    qsTr("Telegram connection link"),
                                                    qsTr("MTProxy connection link"),
                                                    "", "", "")
                                            }
                                        }

                                        ImageButtonType {
                                            implicitWidth: 36
                                            implicitHeight: 36
                                            hoverEnabled: true
                                            image: "qrc:/images/controls/copy.svg"
                                            imageColor: AmneziaStyle.color.paleGray
                                            onClicked: {
                                                GC.copyToClipBoard(settingsRoot.mtProxyTmeLinkForAdditional(modelData))
                                                PageController.showNotificationMessage(qsTr("Copied"))
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: expTgRow.implicitHeight + 16
                                    color: AmneziaStyle.color.onyxBlack
                                    radius: 8
                                    border.color: AmneziaStyle.color.slateGray
                                    border.width: 1

                                    RowLayout {
                                        id: expTgRow
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 8
                                        spacing: 4

                                        CaptionTextType {
                                            Layout.fillWidth: true
                                            text: settingsRoot.mtProxyTgLinkForAdditional(modelData)
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
                                                ExportController.generateQrFromString(settingsRoot.mtProxyTgLinkForAdditional(modelData))
                                                PageController.goToShareConnectionPage(
                                                    qsTr("Telegram connection link"),
                                                    qsTr("MTProxy connection link"),
                                                    "", "", "")
                                            }
                                        }

                                        ImageButtonType {
                                            implicitWidth: 36
                                            implicitHeight: 36
                                            hoverEnabled: true
                                            image: "qrc:/images/controls/copy.svg"
                                            imageColor: AmneziaStyle.color.paleGray
                                            onClicked: {
                                                GC.copyToClipBoard(settingsRoot.mtProxyTgLinkForAdditional(modelData))
                                                PageController.showNotificationMessage(qsTr("Copied"))
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    BasicButtonType {
                        Layout.fillWidth: true
                        Layout.topMargin: 8

                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        text: qsTr("Add additional secret")
                        clickedFunc: function () {
                            MtProxyConfigModel.addAdditionalSecret()
                        }
                    }

                    DividerType {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 8
                    }

                    LabelTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.bottomMargin: 4
                        text: qsTr("Worker mode")
                    }

                    ButtonGroup {
                        id: workerModeGroup
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 4
                        spacing: 0
                        visible: transportMode !== "faketls"

                        HorizontalRadioButton {
                            Layout.fillWidth: true
                            text: qsTr("Auto")
                            ButtonGroup.group: workerModeGroup
                            checked: workersMode === "auto"
                            onClicked: { workersMode = "auto"; MtProxyConfigModel.setWorkersMode("auto") }
                        }
                        HorizontalRadioButton {
                            Layout.fillWidth: true
                            text: qsTr("Manual")
                            ButtonGroup.group: workerModeGroup
                            checked: workersMode === "manual"
                            onClicked: { workersMode = "manual"; MtProxyConfigModel.setWorkersMode("manual") }
                        }
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 8
                        visible: transportMode === "faketls"
                        text: qsTr("Workers are set to 0 automatically for FakeTLS mode.")
                        color: AmneziaStyle.color.mutedGray
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }

                    TextFieldWithHeaderType {
                        id: workersTextField
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        visible: workersMode === "manual" && transportMode !== "faketls"
                        headerText: qsTr("Workers count")
                        textField.placeholderText: "2"
                        textField.text: workers
                        textField.maximumLength: 3
                        textField.validator: IntValidator {
                            bottom: 1
                            top: MtProxyConfigModel.maxWorkers()
                        }
                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            if (textField.text !== workers) {
                                workers = textField.text
                                MtProxyConfigModel.setWorkers(workers)
                            }
                        }
                    }

                    DividerType {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 8
                    }

                    SwitcherType {
                        Layout.fillWidth: true
                        Layout.rightMargin: 16
                        Layout.leftMargin: 16
                        Layout.bottomMargin: 4
                        text: qsTr("Server is behind NAT / Docker bridge")
                        descriptionText: qsTr("Enable if your server is not directly accessible from the internet, e.g. Docker or private network")
                        checked: natEnabled
                        onToggled: function () {
                            if (checked !== natEnabled) {
                                natEnabled = checked
                                MtProxyConfigModel.setNatEnabled(natEnabled)
                            }
                        }
                    }

                    TextFieldWithHeaderType {
                        id: natInternalIpTextField
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        visible: natEnabled
                        headerText: qsTr("Internal IP")
                        textField.placeholderText: "172.17.0.2"
                        textField.text: natInternalIp
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
                            if (!MtProxyConfigModel.isValidOptionalIpv4(textField.text)) {
                                natInternalIpTextField.errorText = qsTr("Enter a valid IPv4 address")
                                return
                            }
                            natInternalIpTextField.errorText = ""
                            if (textField.text !== natInternalIp) {
                                natInternalIp = textField.text
                                MtProxyConfigModel.setNatInternalIp(natInternalIp)
                            }
                        }
                    }

                    TextFieldWithHeaderType {
                        id: natExternalIpTextField
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        visible: natEnabled
                        headerText: qsTr("External IP")
                        textField.placeholderText: "1.2.3.4"
                        textField.text: natExternalIp
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
                            if (!MtProxyConfigModel.isValidOptionalIpv4(textField.text)) {
                                natExternalIpTextField.errorText = qsTr("Enter a valid IPv4 address")
                                return
                            }
                            natExternalIpTextField.errorText = ""
                            if (textField.text !== natExternalIp) {
                                natExternalIp = textField.text
                                MtProxyConfigModel.setNatExternalIp(natExternalIp)
                            }
                        }
                    }
                }

                DividerType {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    spacing: 8
                    visible: containerStatus === 1

                    RowLayout {
                        Layout.fillWidth: true

                        Header2Type {
                            Layout.fillWidth: true
                            headerText: qsTr("Diagnostics")
                        }

                        ImageButtonType {
                            implicitWidth: 32
                            implicitHeight: 32
                            image: "qrc:/images/controls/refresh-cw.svg"
                            imageColor: diagLoading ? AmneziaStyle.color.mutedGray : AmneziaStyle.color.paleGray
                            hoverEnabled: !diagLoading
                            enabled: !diagLoading
                            onClicked: {
                                diagLoading = true
                                InstallController.refreshContainerDiagnostics(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, parseInt(port))
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
                            color: diagClientsConnected >= 0 ? (diagPortReachable ? AmneziaStyle.color.paleGray : AmneziaStyle.color.vibrantRed) : AmneziaStyle.color.mutedGray
                        }
                        CaptionTextType {
                            Layout.fillWidth: true
                            text: qsTr("Public port reachable")
                            color: AmneziaStyle.color.paleGray
                        }
                        CaptionTextType {
                            text: diagClientsConnected < 0 ? qsTr("—") : (diagPortReachable ? qsTr("Yes") : qsTr("No"))
                            color: diagClientsConnected >= 0 ? (diagPortReachable ? AmneziaStyle.color.paleGray : AmneziaStyle.color.vibrantRed) : AmneziaStyle.color.mutedGray
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: diagClientsConnected >= 0 ? (diagTelegramReachable ? AmneziaStyle.color.paleGray : AmneziaStyle.color.vibrantRed) : AmneziaStyle.color.mutedGray
                        }
                        CaptionTextType {
                            Layout.fillWidth: true
                            text: qsTr("Telegram upstream reachable")
                            color: AmneziaStyle.color.paleGray
                        }
                        CaptionTextType {
                            text: diagClientsConnected < 0 ? qsTr("—") : (diagTelegramReachable ? qsTr("Yes") : qsTr("No"))
                            color: diagClientsConnected >= 0 ? (diagTelegramReachable ? AmneziaStyle.color.paleGray : AmneziaStyle.color.vibrantRed) : AmneziaStyle.color.mutedGray
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: diagClientsConnected >= 0 ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.mutedGray
                        }
                        CaptionTextType {
                            Layout.fillWidth: true
                            text: qsTr("Clients connected")
                            color: AmneziaStyle.color.paleGray
                        }
                        CaptionTextType {
                            text: diagClientsConnected < 0 ? qsTr("—") : diagClientsConnected.toString()
                            color: AmneziaStyle.color.paleGray
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: AmneziaStyle.color.mutedGray
                        }
                        CaptionTextType {
                            Layout.fillWidth: true
                            text: qsTr("Last config refresh")
                            color: AmneziaStyle.color.paleGray
                        }
                        CaptionTextType {
                            text: diagLastConfigRefresh !== "" ? diagLastConfigRefresh : qsTr("—")
                            color: AmneziaStyle.color.mutedGray
                        }
                    }

                    LabelWithButtonType {
                        Layout.fillWidth: true
                        Layout.leftMargin: -16
                        visible: diagStatsEndpoint !== ""
                        text: qsTr("Stats endpoint")
                        descriptionText: diagStatsEndpoint
                        descriptionOnTop: true
                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: AmneziaStyle.color.paleGray
                        clickedFunction: function () {
                            GC.copyToClipBoard(diagStatsEndpoint)
                            PageController.showNotificationMessage(qsTr("Copied"))
                        }
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: diagLoading ? qsTr("Refreshing…") : qsTr("Tap ↻ to refresh diagnostics")
                        color: AmneziaStyle.color.mutedGray
                        visible: diagClientsConnected < 0
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16 * 2
                    Layout.bottomMargin: 24
                    text: qsTr("If you change the settings, the proxy connection link will change. The old link will stop working.")
                    color: AmneziaStyle.color.mutedGray
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                }

                BasicButtonType {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.bottomMargin: 32
                    Layout.rightMargin: 16
                    Layout.leftMargin: 16
                    visible: ServersUiController.isProcessedServerHasWriteAccess()
                    enabled: !root.mtProxyNetworkBlocked
                    text: qsTr("Save")
                    clickedFunc: function () {
                        if (root.mtProxyNetworkBlocked) {
                            PageController.showErrorMessage(qsTr("No internet connection. Connect to the internet to change MTProxy settings."))
                            return
                        }
                        publicHostTextField.errorText = ""
                        tagTextField.errorText = ""
                        tlsDomainTextField.errorText = ""
                        natInternalIpTextField.errorText = ""
                        natExternalIpTextField.errorText = ""
                        portTextField.errorText = ""

                        var portValue = portTextField.textField.text === ""
                            ? MtProxyConfigModel.defaultPort()
                            : portTextField.textField.text

                        var errorLines = []
                        var bullet = "- "
                        if (!portTextField.textField.acceptableInput && portTextField.textField.text !== "") {
                            var portErr = qsTr("The port must be in the range of 1 to 65535")
                            portTextField.errorText = portErr
                            errorLines.push(bullet + portErr)
                        }
                        if (!MtProxyConfigModel.isValidPublicHost(publicHostTextField.textField.text)) {
                            var hostErr = qsTr("Enter a valid IP address or domain name")
                            publicHostTextField.errorText = hostErr
                            errorLines.push(bullet + hostErr)
                        }
                        var tagNormalized = MtProxyConfigModel.sanitizeMtProxyTagFieldText(tagTextField.textField.text)
                        tagTextField.textField.text = tagNormalized
                        if (!MtProxyConfigModel.isValidMtProxyTag(tagNormalized)) {
                            var tagErr = qsTr("Proxy tag must be exactly 32 hexadecimal characters (0-9, A-F), or leave empty.")
                            tagTextField.errorText = tagErr
                            errorLines.push(bullet + tagErr)
                        }
                        var domainValueForSave = tlsDomainTextField.textField.text === ""
                            ? MtProxyConfigModel.defaultTlsDomain()
                            : tlsDomainTextField.textField.text
                        if (!MtProxyConfigModel.isValidFakeTlsDomain(domainValueForSave)) {
                            var tlsErr = qsTr("Enter a valid domain name")
                            tlsDomainTextField.errorText = tlsErr
                            errorLines.push(bullet + tlsErr)
                        }
                        var natIpErr = qsTr("Enter a valid IPv4 address")
                        if (!MtProxyConfigModel.isValidOptionalIpv4(natInternalIpTextField.textField.text)) {
                            natInternalIpTextField.errorText = natIpErr
                            errorLines.push(bullet + qsTr("NAT internal IP: enter a valid IPv4 address"))
                        }
                        if (!MtProxyConfigModel.isValidOptionalIpv4(natExternalIpTextField.textField.text)) {
                            natExternalIpTextField.errorText = natIpErr
                            errorLines.push(bullet + qsTr("NAT external IP: enter a valid IPv4 address"))
                        }
                        if (errorLines.length > 0) {
                            PageController.showErrorMessage(errorLines.join("\n"))
                            return
                        }
                        MtProxyConfigModel.setPort(portValue)
                        MtProxyConfigModel.setTag(tagNormalized)
                        MtProxyConfigModel.setPublicHost(publicHostTextField.textField.text)
                        MtProxyConfigModel.setTransportMode(transportMode)
                        var domainValue = tlsDomainTextField.textField.text === ""
                            ? MtProxyConfigModel.defaultTlsDomain()
                            : tlsDomainTextField.textField.text
                        MtProxyConfigModel.setTlsDomain(domainValue)

                        if (transportMode === "faketls") {
                            workers = "0"
                            MtProxyConfigModel.setWorkers("0")
                        } else {
                            MtProxyConfigModel.setWorkersMode(workersMode)
                            MtProxyConfigModel.setWorkers(workers)
                        }
                        MtProxyConfigModel.setNatEnabled(natEnabled)
                        MtProxyConfigModel.setNatInternalIp(natInternalIpTextField.textField.text)
                        MtProxyConfigModel.setNatExternalIp(natExternalIpTextField.textField.text)

                        previousPort = port
                        previousTag = tag
                        previousPublicHost = publicHost
                        previousTransportMode = transportMode
                        previousTlsDomain = tlsDomain
                        previousWorkersMode = workersMode
                        previousWorkers = workers
                        previousNatEnabled = natEnabled
                        previousNatInternalIp = natInternalIp
                        previousNatExternalIp = natExternalIp
                        root.previousSecret = secret
                        isUpdating = true
                        root.mtProxyScheduleUpdate(false)
                    }
                }
            }
        }
    }

    }
}
