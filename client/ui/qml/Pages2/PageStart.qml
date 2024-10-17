import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: homeTabButton

    property bool isControlsDisabled: false
    property bool isTabBarDisabled: false

    Connections {
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

        function onForceTabBarActiveFocus() {
            homeTabButton.focus = true
            tabBar.forceActiveFocus()
        }

        function onForceStackActiveFocus() {
            homeTabButton.focus = true
            tabBarStackView.forceActiveFocus()
        }
    }

    Connections {
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

        function onApiConfigRemoved(message) {
            PageController.showNotificationMessage(message)
        }

        function onInstallServerFromApiFinished(message) {
            PageController.showBusyIndicator(false)
            if (!ConnectionController.isConnected) {
                ServersModel.setDefaultServerIndex(ServersModel.getServersCount() - 1);
                ServersModel.processedIndex = ServersModel.defaultIndex
            }

            PageController.goToPageHome()
            PageController.showNotificationMessage(message)
        }

        function onChangeApiCountryFinished(message) {
            PageController.showBusyIndicator(false)

            PageController.goToPageHome()
            PageController.showNotificationMessage(message)
        }

        function onReloadServerFromApiFinished(message) {
            PageController.goToPageHome()
            PageController.showNotificationMessage(message)
        }
    }

    Connections {
        target: ConnectionController

        function onReconnectWithUpdatedContainer(message) {
            PageController.showNotificationMessage(message)
            PageController.closePage()
        }

        function onNoInstalledContainers() {
            PageController.setTriggeredByConnectButton(true)

            ServersModel.processedIndex = ServersModel.getDefaultServerIndex()
            InstallController.setShouldCreateServer(false)
            PageController.goToPage(PageEnum.PageSetupWizardEasy)
        }
    }

    Connections {
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

    StackViewType {
        id: tabBarStackView

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: tabBar.top

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
                ServersModel.processedIndex = ServersModel.defaultIndex
            }

            tabBarStackView.push(pagePath, { "objectName" : pagePath })
        }

        Keys.onPressed: function(event) {
            PageController.keyPressEvent(event.key)
            event.accepted = true
        }
    }

    TabBar {
        id: tabBar

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        topPadding: 8
        bottomPadding: 8
        leftPadding: 96
        rightPadding: 96

        height: visible ? homeTabButton.implicitHeight + tabBar.topPadding + tabBar.bottomPadding : 0

        enabled: !root.isControlsDisabled && !root.isTabBarDisabled

        background: Shape {
            width: parent.width
            height: parent.height

            ShapePath {
                startX: 0
                startY: 0

                PathLine { x: width; y: 0 }
                PathLine { x: width; y: tabBar.height - 1 }
                PathLine { x: 0; y: tabBar.height - 1 }
                PathLine { x: 0; y: 0 }

                strokeWidth: 1
                strokeColor: AmneziaStyle.color.slateGray
                fillColor: AmneziaStyle.color.onyxBlack
            }
        }

        TabImageButtonType {
            id: homeTabButton
            isSelected: tabBar.currentIndex === 0
            image: "qrc:/images/controls/home.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageHome)
                ServersModel.processedIndex = ServersModel.defaultIndex
                tabBar.currentIndex = 0
            }

            KeyNavigation.tab: shareTabButton
            Keys.onEnterPressed: this.clicked()
            Keys.onReturnPressed: this.clicked()
        }

        TabImageButtonType {
            id: shareTabButton

            Connections {
                target: ServersModel

                function onModelReset() {
                    var hasServerWithWriteAccess = ServersModel.hasServerWithWriteAccess()
                    shareTabButton.visible = hasServerWithWriteAccess
                    shareTabButton.width = hasServerWithWriteAccess ? undefined : 0
                }
            }

            visible: ServersModel.hasServerWithWriteAccess()
            width: ServersModel.hasServerWithWriteAccess() ? undefined : 0

            isSelected: tabBar.currentIndex === 1
            image: "qrc:/images/controls/share-2.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageShare)
                tabBar.currentIndex = 1
            }

            KeyNavigation.tab: settingsTabButton
        }

        TabImageButtonType {
            id: settingsTabButton
            isSelected: tabBar.currentIndex === 2
            image: "qrc:/images/controls/settings-2.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageSettings)
                tabBar.currentIndex = 2
            }

            KeyNavigation.tab: plusTabButton
        }

        TabImageButtonType {
            id: plusTabButton
            isSelected: tabBar.currentIndex === 3
            image: "qrc:/images/controls/plus.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageSetupWizardConfigSource)
                tabBar.currentIndex = 3
            }

            Keys.onTabPressed: PageController.forceStackActiveFocus()
        }
    }
}
