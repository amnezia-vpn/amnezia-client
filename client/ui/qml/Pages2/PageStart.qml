import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Window

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool isControlsDisabled: false
    property bool isTabBarDisabled: false

    /** Loud colors (tab bar base green, extra overlap) when true — pair with PageSettingsApiQrPairingSend.pairingQrChromeDebug. */
    property bool pairingQrChromeDebug: false

    /** Opaque extension of tab bar background upward (iOS embedded QR); see PairingTabChrome deltaY vs stack bottom. */
    readonly property int tabBarChromeOverlapUp: (PairingUiController.embeddedPairingQrCameraActive && GC.isMobile()
                                                  && Qt.platform.os !== "android")
                                                 ? (root.pairingQrChromeDebug ? 24 : 18) : 0

    /** Pull stack under tab chrome so TabBar.background overlap fully covers stack bottom pixels. */
    readonly property int tabStackPairingUnderlapDown: (PairingUiController.embeddedPairingQrCameraActive && GC.isMobile()
                                                       && Qt.platform.os !== "android") ? 8 : 0

    readonly property bool pairingTabChromeLogActive: PairingUiController.embeddedPairingQrCameraActive && GC.isMobile()
                                                        && Qt.platform.os !== "android"

    function logPairingTabChromeLayout(tag) {
        if (!root.pairingTabChromeLogActive) {
            return
        }
        const w = Window.window
        const ci = w && w.contentItem ? w.contentItem : null
        let msg = "[PairingTabChrome] " + tag
        msg += " PageStart=" + Math.round(root.width) + "x" + Math.round(root.height)
        msg += " tabBar=" + Math.round(tabBar.width) + "x" + Math.round(tabBar.height) + " y=" + tabBar.y.toFixed(2)
        msg += " imeBM=" + PageController.imeHeight
        msg += " stack=" + Math.round(tabBarStackView.width) + "x" + Math.round(tabBarStackView.height)
        msg += " overlapUp=" + tabBarChromeOverlapUp + " stackUnderlap=" + tabStackPairingUnderlapDown
        msg += " tabBgRootH=" + (tabBarBackgroundRoot ? tabBarBackgroundRoot.height.toFixed(2) : "n/a")
        if (ci) {
            const tabOrigin = tabBar.mapToItem(ci, 0, 0)
            const tabBandTop = tabBar.mapToItem(ci, 0, -tabBarChromeOverlapUp)
            const stackOrigin = tabBarStackView.mapToItem(ci, 0, 0)
            const stackBottomMid = tabBarStackView.mapToItem(ci, tabBarStackView.width * 0.5, tabBarStackView.height)
            const bgTopLeft = tabBarBackgroundRoot.mapToItem(ci, 0, 0)
            msg += " ci.tab(0,0)=" + tabOrigin.x.toFixed(1) + "," + tabOrigin.y.toFixed(1)
            msg += " ci.tab(0,-ov)=" + tabBandTop.x.toFixed(1) + "," + tabBandTop.y.toFixed(1)
            msg += " ci.stack(0,0)=" + stackOrigin.x.toFixed(1) + "," + stackOrigin.y.toFixed(1)
            msg += " ci.stackMidBot=" + stackBottomMid.x.toFixed(1) + "," + stackBottomMid.y.toFixed(1)
            msg += " ci.tabBgRoot(0,0)=" + bgTopLeft.x.toFixed(1) + "," + bgTopLeft.y.toFixed(1)
            msg += " deltaY_tabBandTop_minus_stackMidBot=" + (tabBandTop.y - stackBottomMid.y).toFixed(2)
        } else {
            msg += " ci=missing"
        }
        console.warn(msg)
    }

    Timer {
        id: pairingTabChromeLogTimer50
        interval: 50
        repeat: false
        onTriggered: root.logPairingTabChromeLayout("t50")
    }
    Timer {
        id: pairingTabChromeLogTimer350
        interval: 350
        repeat: false
        onTriggered: root.logPairingTabChromeLayout("t350")
    }

    Connections {
        target: PairingUiController

        function onEmbeddedPairingQrCameraActiveChanged() {
            if (PairingUiController.embeddedPairingQrCameraActive && GC.isMobile() && Qt.platform.os !== "android") {
                pairingTabChromeLogTimer50.restart()
                pairingTabChromeLogTimer350.restart()
            }
        }
    }

    onWidthChanged: {
        if (root.pairingTabChromeLogActive) {
            pairingTabChromeLogTimer50.restart()
        }
    }
    onHeightChanged: {
        if (root.pairingTabChromeLogActive) {
            pairingTabChromeLogTimer50.restart()
        }
    }

    Connections {
        objectName: "pageControllerConnection"

        target: PageController

        function onGoToPageHome() {
            if (PageController.isStartPageVisible()) {
                tabBar.visible = false
                tabBarStackView.goToTabBarPage(PageEnum.PageSetupWizardStart)
            } else {
                tabBar.visible = true
                tabBar.setCurrentIndex(0)
                tabBarStackView.goToTabBarPage(PageEnum.PageHome)
            }
        }

        function onGoToPageSettings() {
            tabBar.setCurrentIndex(2)
            tabBarStackView.goToTabBarPage(PageEnum.PageSettings)
        }

        function onGoToPageViewConfig() {
            var pagePath = PageController.getPagePath(PageEnum.PageSetupWizardViewConfig)
            tabBarStackView.push(pagePath, { "objectName" : pagePath }, StackView.PushTransition)
        }

        function onGoToShareConnectionPage(headerText, configContentHeaderText, configCaption, configExtension, configFileName) {
            var pagePath = PageController.getPagePath(PageEnum.PageShareConnection)
            tabBarStackView.push(pagePath,
                                 { "objectName" : pagePath,
                                     "headerText" : headerText,
                                     "configContentHeaderText" : configContentHeaderText,
                                     "configCaption" : configCaption,
                                     "configExtension" : configExtension,
                                     "configFileName" : configFileName
                                 },
                                 StackView.PushTransition)
        }

        function onDisableControls(disabled) {
            isControlsDisabled = disabled
        }

        function onDisableTabBar(disabled) {
            isTabBarDisabled = disabled
        }

        function onClosePage() {
            if (tabBarStackView.depth <= 1) {
                PageController.hideWindow()
                return
            }
            tabBarStackView.pop()
        }

        function onGoToPage(page, slide) {
            var pagePath = PageController.getPagePath(page)

            if (slide) {
                tabBarStackView.push(pagePath, { "objectName" : pagePath }, StackView.PushTransition)
            } else {
                tabBarStackView.push(pagePath, { "objectName" : pagePath }, StackView.Immediate)
            }
        }

        function onGoToStartPage() {
            while (tabBarStackView.depth > 1) {
                tabBarStackView.pop()
            }
        }

        function onEscapePressed() {
            if (root.isControlsDisabled || root.isTabBarDisabled) {
                return
            }

            var pageName = tabBarStackView.currentItem.objectName
            if ((pageName === PageController.getPagePath(PageEnum.PageShare)) ||
                    (pageName === PageController.getPagePath(PageEnum.PageSettings)) ||
                    (pageName === PageController.getPagePath(PageEnum.PageSetupWizardConfigSource))) {
                PageController.goToPageHome()
            } else {
                PageController.closePage()
            }
        }
    }

    Connections {
        objectName: "installControllerConnections"

        target: InstallController

        function onInstallationErrorOccurred(error) {
            PageController.showBusyIndicator(false)

            PageController.showErrorMessage(error)

            var needCloseCurrentPage = false
            var currentPageName = tabBarStackView.currentItem.objectName

            if (currentPageName === PageController.getPagePath(PageEnum.PageSetupWizardInstalling)) {
                needCloseCurrentPage = true
            } else if (currentPageName === PageController.getPagePath(PageEnum.PageDeinstalling)) {
                needCloseCurrentPage = true
            }
            if (needCloseCurrentPage) {
                PageController.closePage()
            }
        }

        function onWrongInstallationUser(message) {
            onInstallationErrorOccurred(message)
        }

        function onUpdateContainerFinished(message) {
            PageController.showNotificationMessage(message)
            PageController.closePage()
        }

        function onCachedProfileCleared(message) {
            PageController.showNotificationMessage(message)
        }

        function onRemoveServerFinished(finishedMessage) {
            if (!ServersModel.getServersCount()) {
                PageController.goToPageHome()
            } else {
                PageController.goToStartPage()
                PageController.goToPage(PageEnum.PageSettingsServersList)
            }
            PageController.showNotificationMessage(finishedMessage)
        }

        function onNoInstalledContainers() {
            PageController.setTriggeredByConnectButton(true)

            ServersUiController.processedIndex = ServersUiController.defaultIndex
            PageController.goToPage(PageEnum.PageSetupWizardEasy)
        }
    }

    Connections {
        objectName: "connectionControllerConnections"

        target: ConnectionController

        function onReconnectWithUpdatedContainer(message) {
            PageController.showNotificationMessage(message)
            PageController.closePage()
        }
    }

    Connections {
        objectName: "importControllerConnections"

        target: ImportController

        function onImportErrorOccurred(error, goToPageHome) {
            PageController.showErrorMessage(error)
        }

        function onRestoreAppConfig(data) {
            PageController.showBusyIndicator(true)
            SettingsController.restoreAppConfigFromData(data)
            PageController.showBusyIndicator(false)
        }
    }

    Connections {
        objectName: "settingsControllerConnections"

        target: SettingsController

        function onLoggingDisableByWatcher() {
            PageController.showNotificationMessage(qsTr("Logging was disabled after 14 days, log files were deleted"))
        }

        function onRestoreBackupFinished() {
            PageController.showNotificationMessage(qsTr("Settings restored from backup file"))
            PageController.goToPageHome()
        }

        function onLoggingStateChanged() {
            if (SettingsController.isLoggingEnabled) {
                var message = qsTr("Logging is enabled. Note that logs will be automatically" +
                                   "disabled after 14 days, and all log files will be deleted.")
                PageController.showNotificationMessage(message)
            }
        }
    }

    Connections {
        target: SubscriptionUiController

        function onErrorOccurred(error) {
            PageController.showErrorMessage(error)
        }
    }

    Connections {
        target: SubscriptionUiController

        function onApiConfigRemoved(message) {
            PageController.showNotificationMessage(message)
        }

        function onInstallServerFromApiFinished(message, preferredDefaultIndex) {
            if (!ConnectionController.isConnected) {
                if (preferredDefaultIndex !== undefined && preferredDefaultIndex >= 0) {
                    ServersUiController.setDefaultServerIndex(preferredDefaultIndex)
                } else {
                    ServersUiController.setDefaultServerIndex(ServersModel.getServersCount() - 1);
                }
                ServersUiController.processedIndex = ServersUiController.defaultIndex
            }

            PageController.goToPageHome()
            PageController.showNotificationMessage(message)
        }

        function onChangeApiCountryFinished(message) {
            PageController.goToPageHome()
            PageController.showNotificationMessage(message)
        }

        function onReloadServerFromApiFinished(message) {
            PageController.goToPageHome()
            PageController.showNotificationMessage(message)
        }
    }

    StackViewType {
        id: tabBarStackView
        objectName: "tabBarStackView"

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: tabBar.top
        anchors.bottomMargin: -root.tabStackPairingUnderlapDown

        enabled: !root.isControlsDisabled

        function goToTabBarPage(page) {
            var pagePath = PageController.getPagePath(page)
            tabBarStackView.clear(StackView.Immediate)
            tabBarStackView.replace(pagePath, { "objectName" : pagePath }, StackView.Immediate)
        }

        Component.onCompleted: {
            var pagePath
            if (PageController.isStartPageVisible()) {
                tabBar.visible = false
                pagePath = PageController.getPagePath(PageEnum.PageSetupWizardStart)
            } else {
                tabBar.visible = true
                pagePath = PageController.getPagePath(PageEnum.PageHome)
                ServersUiController.processedIndex = ServersUiController.defaultIndex
            }

            tabBarStackView.push(pagePath, { "objectName" : pagePath })
        }

        Keys.onPressed: function(event) {
            switch (event.key) {
            case Qt.Key_Tab:
            case Qt.Key_Down:
            case Qt.Key_Right:
                FocusController.nextKeyTabItem()
                break
            case Qt.Key_Backtab:
            case Qt.Key_Up:
            case Qt.Key_Left:
                FocusController.previousKeyTabItem()
                break
            default:
                PageController.keyPressEvent(event.key)
                event.accepted = true
            }
        }
    }

    TabBar {
        id: tabBar
        objectName: "tabBar"

        clip: false

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        // Also adjust TabBar position when keyboard appears (Android 14+ workaround)
        anchors.bottomMargin: PageController.imeHeight

        topPadding: 8
        bottomPadding: 8 + PageController.safeAreaBottomMargin
        leftPadding: 96
        rightPadding: 96

        height: visible ? homeTabButton.implicitHeight + tabBar.topPadding + tabBar.bottomPadding : 0

        enabled: !root.isControlsDisabled && !root.isTabBarDisabled

        background: Item {
            id: tabBarBackgroundRoot
            objectName: "tabBarBackgroundRoot"

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height + root.tabBarChromeOverlapUp

            /** Opaque base: Shape alone can show the window-layer camera through anti-aliased edges when the window is clear (iOS QR pairing). */
            Rectangle {
                anchors.fill: parent
                color: root.pairingQrChromeDebug ? "#00ff66" : AmneziaStyle.color.onyxBlack
            }
            /** Stroke around tab row; hidden during iOS embedded QR overlap — top horizontal slateGray reads as a hairline “strip” above tabs. */
            Shape {
                id: tabBarChromeShape
                objectName: "backgroundShape"
                visible: root.tabBarChromeOverlapUp === 0
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: tabBar.height

                ShapePath {
                    startX: 0
                    startY: 0

                    PathLine { x: tabBarChromeShape.width; y: 0 }
                    PathLine { x: tabBarChromeShape.width; y: tabBarChromeShape.height - 1 }
                    PathLine { x: 0; y: tabBarChromeShape.height - 1 }
                    PathLine { x: 0; y: 0 }

                    strokeWidth: 1
                    strokeColor: AmneziaStyle.color.slateGray
                    fillColor: "transparent"
                }
            }
        }

        TabImageButtonType {
            id: homeTabButton
            objectName: "homeTabButton"

            isSelected: tabBar.currentIndex === 0
            image: "qrc:/images/controls/home.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageHome)
                ServersUiController.processedIndex = ServersUiController.defaultIndex
                tabBar.currentIndex = 0
            }
        }

        TabImageButtonType {
            id: shareTabButton
            objectName: "shareTabButton"

            Connections {
                target: ServersModel

                function onModelReset() {
                    if (!SettingsController.isOnTv()) {
                        var hasServerWithWriteAccess = ServersModel.hasServerWithWriteAccess()
                        shareTabButton.visible = hasServerWithWriteAccess
                        shareTabButton.width = hasServerWithWriteAccess ? undefined : 0
                    }
                }
            }

            visible: !SettingsController.isOnTv() && ServersModel.hasServerWithWriteAccess()
            width: !SettingsController.isOnTv() && ServersModel.hasServerWithWriteAccess() ? undefined : 0

            isSelected: tabBar.currentIndex === 1
            image: "qrc:/images/controls/share-2.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageShare)
                tabBar.currentIndex = 1
            }
        }

        TabImageButtonType {
            id: settingsTabButton
            objectName: "settingsTabButton"

            isSelected: tabBar.currentIndex === 2
            image: (ServersUiController.hasServersFromGatewayApi && NewsModel.hasUnread && SettingsController.isNewsNotificationsEnabled()) ? "qrc:/images/controls/settings-news.svg" : "qrc:/images/controls/settings.svg"
            Binding {
                target: settingsTabButton
                property: "defaultColor"
                value: "transparent"
                when: (ServersUiController.hasServersFromGatewayApi && NewsModel.hasUnread)
            }
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageSettings)
                tabBar.currentIndex = 2
            }
        }

        TabImageButtonType {
            id: plusTabButton
            objectName: "plusTabButton"

            isSelected: tabBar.currentIndex === 3
            image: "qrc:/images/controls/plus.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageSetupWizardConfigSource)
                tabBar.currentIndex = 3
            }
        }
    }
}
