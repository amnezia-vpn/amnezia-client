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

    property bool isControlsDisabled: false
    property bool isTabBarDisabled: false
    property bool mobileTabBarVisible: true
    readonly property bool tvInterfaceActive: SettingsController.isTvInterfaceActive

    Loader {
        id: tvRootLoader
        anchors.fill: parent
        active: root.tvInterfaceActive
        visible: active
        source: active ? "PageTvRoot.qml" : ""
        onStatusChanged: console.log("TV root loader status:", status, "source:", source)
    }

    Rectangle {
        anchors.fill: parent
        visible: root.tvInterfaceActive && tvRootLoader.status === Loader.Error
        color: "#08090B"
        z: 100

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 96, 720)
            spacing: 14

            Label {
                Layout.fillWidth: true
                text: "TV UI failed to load"
                color: "#F8FAFC"
                font.pixelSize: 36
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                Layout.fillWidth: true
                text: "Source: " + tvRootLoader.source + "\nStatus: " + tvRootLoader.status
                color: "#A1A1AA"
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
    }

    Connections {
        objectName: "pageControllerConnection"

        target: PageController

        function onGoToPageHome() {
            if (PageController.isStartPageVisible() && !FBLinkController.isLoggedIn) {
                root.mobileTabBarVisible = false
                tabBarStackView.goToTabBarPage(PageEnum.PageFBLinkLogin)
            } else {
                root.mobileTabBarVisible = true
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

        function onApiConfigRemoved(message) {
            PageController.showNotificationMessage(message)
        }

        function onRemoveProcessedServerFinished(finishedMessage) {
            if (!ServersModel.getServersCount()) {
                PageController.goToPageHome()
            } else {
                PageController.goToStartPage()
                PageController.goToPage(PageEnum.PageSettingsServersList)
            }
            PageController.showNotificationMessage(finishedMessage)
        }

        function onNoInstalledContainers() {
            // Adding servers manually is disabled — redirect to login/subscription
            if (!FBLinkController.isLoggedIn) {
                tabBarStackView.goToTabBarPage(PageEnum.PageFBLinkLogin)
                root.mobileTabBarVisible = false
            } else if (!FBLinkController.isSubscribed) {
                root.mobileTabBarVisible = true
                tabBar.setCurrentIndex(0)
                tabBarStackView.goToTabBarPage(PageEnum.PageHome)
            } else {
                // Logged in with subscription but no containers: fetch fresh config
                FBLinkController.fetchConfig()
            }
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
        target: ApiSettingsController

        function onErrorOccurred(error) {
            PageController.showErrorMessage(error)
        }
    }

    Connections {
        target: ApiConfigsController

        function onInstallServerFromApiFinished(message) {
            if (!ConnectionController.isConnected) {
                ServersModel.setDefaultServerIndex(ServersModel.getServersCount() - 1);
                ServersModel.processedIndex = ServersModel.defaultIndex
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

        visible: !root.tvInterfaceActive
        enabled: !root.tvInterfaceActive && !root.isControlsDisabled

        function goToTabBarPage(page) {
            var pagePath = PageController.getPagePath(page)
            tabBarStackView.clear(StackView.Immediate)
            tabBarStackView.replace(pagePath, { "objectName" : pagePath }, StackView.Immediate)
        }

        Component.onCompleted: {
            if (root.tvInterfaceActive) {
                console.log("PageStart: TV interface active, skipping mobile stack")
                return
            }
            var pagePath
            if (PageController.isStartPageVisible() && !FBLinkController.isLoggedIn) {
                root.mobileTabBarVisible = false
                pagePath = PageController.getPagePath(PageEnum.PageFBLinkLogin)
            } else {
                root.mobileTabBarVisible = true
                pagePath = PageController.getPagePath(PageEnum.PageHome)
                ServersModel.processedIndex = ServersModel.defaultIndex
            }

            tabBarStackView.push(pagePath, { "objectName" : pagePath })
        }

        Keys.onPressed: function(event) {
            console.debug(">>>> ", event.key, " Event is caught by StartPage")
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
        visible: !root.tvInterfaceActive && root.mobileTabBarVisible

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        
        // Also adjust TabBar position when keyboard appears (Android 14+ workaround)
        anchors.bottomMargin: SettingsController.imeHeight

        topPadding: 8
        bottomPadding: 8 + SettingsController.safeAreaBottomMargin
        leftPadding: 72
        rightPadding: 72

        height: visible ? homeTabButton.implicitHeight + tabBar.topPadding + tabBar.bottomPadding : 0

        enabled: !root.isControlsDisabled && !root.isTabBarDisabled

        background: Shape {
            objectName: "backgroundShape"

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
                strokeColor: FBLinkStyle.color.slateGray
                fillColor: FBLinkStyle.color.onyxBlack
            }
        }

        TabImageButtonType {
            id: homeTabButton
            objectName: "homeTabButton"

            isSelected: tabBar.currentIndex === 0
            image: "qrc:/images/controls/home.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageHome)
                ServersModel.processedIndex = ServersModel.defaultIndex
                tabBar.currentIndex = 0
            }
        }

        TabImageButtonType {
            id: locationsTabButton
            objectName: "locationsTabButton"

            isSelected: tabBar.currentIndex === 1
            image: "qrc:/images/controls/map-pin.svg"
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageSettingsServersList)
                tabBar.currentIndex = 1
            }
        }

        TabImageButtonType {
            id: settingsTabButton
            objectName: "settingsTabButton"

            isSelected: tabBar.currentIndex === 2
            image: (ServersModel.hasServersFromGatewayApi && NewsModel.hasUnread && SettingsController.isNewsNotificationsEnabled()) ? "qrc:/images/controls/settings-news.svg" : "qrc:/images/controls/settings.svg"
            Binding {
                target: settingsTabButton
                property: "defaultColor"
                value: "transparent"
                when: (ServersModel.hasServersFromGatewayApi && NewsModel.hasUnread)
            }
            clickedFunc: function () {
                tabBarStackView.goToTabBarPage(PageEnum.PageSettings)
                tabBar.currentIndex = 2
            }
        }


    }
}
