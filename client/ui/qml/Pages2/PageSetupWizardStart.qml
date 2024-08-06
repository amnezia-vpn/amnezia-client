import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    property bool isControlsDisabled: false

    defaultActiveFocusItem: startButton

    Connections {
        target: PageController

        function onGoToPageViewConfig() {
            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
        }

        function onClosePage() {
            if (stackView.depth <= 1) {
                PageController.hideWindow()
                return
            }
            stackView.pop()
        }

        function onGoToPage(page, slide) {
            var pagePath = PageController.getPagePath(page)
            if (slide) {
                stackView.push(pagePath, { "objectName" : pagePath }, StackView.PushTransition)
            } else {
                stackView.push(pagePath, { "objectName" : pagePath }, StackView.Immediate)
            }
        }

        function onGoToStartPage() {
            while (stackView.depth > 1) {
                stackView.pop()
            }
        }

        function onDisableControls(disabled) {
            isControlsDisabled = disabled
        }

        function onDisableTabBar(disabled) {
            isControlsDisabled = disabled
        }

        function onEscapePressed() {
            if (isControlsDisabled) {
                return
            }

            PageController.closePage()
        }
    }

    Connections {
        target: SettingsController

        function onRestoreBackupFinished() {
            PageController.showNotificationMessage(qsTr("Settings restored from backup file"))
            PageController.replaceStartPage()
        }
    }

    Connections {
        target: InstallController

        function onInstallationErrorOccurred(error) {
            PageController.showBusyIndicator(false)
            PageController.showErrorMessage(error)

            var currentPageName = stackView.currentItem.objectName

            if (currentPageName === PageController.getPagePath(PageEnum.PageSetupWizardInstalling)) {
                PageController.closePage()
            }
        }
    }

    Connections {
        target: ImportController

        function onRestoreAppConfig(data) {
            PageController.showBusyIndicator(true)
            SettingsController.restoreAppConfigFromData(data)
            PageController.showBusyIndicator(false)
        }

        function onImportErrorOccurred(error, goToPageHome) {
            PageController.showErrorMessage(error)
        }
    }

    FlickableType {
        id: fl
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 0

            Image {
                id: image
                source: "qrc:/images/amneziaBigLogo.png"

                Layout.alignment: Qt.AlignCenter
                Layout.topMargin: 32
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.preferredWidth: 344
                Layout.preferredHeight: 279
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 50
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Free service for creating a personal VPN on your server.") +
                      qsTr(" Helps you access blocked content without revealing your privacy, even to VPN providers.")
            }

            BasicButtonType {
                id: startButton
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("I have the data to connect")

                clickedFunc: function() {
                    connectionTypeSelection.open()
                }
            }

            BasicButtonType {
                id: startButton2
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.blackHovered
                pressedColor: AmneziaStyle.color.blackPressed
                disabledColor: AmneziaStyle.color.grey
                textColor: AmneziaStyle.color.white
                borderWidth: 1

                text: qsTr("I have nothing")

                clickedFunc: function() {
                    Qt.openUrlExternally(qsTr("https://amnezia.org/instructions/0_starter-guide"))
                }
            }
        }
    }

    ConnectionTypeSelectionDrawer {
        id: connectionTypeSelection
        parentPage: root

        onClosed: {
        //     PageController.forceTabBarActiveFocus()
        //     root.defaultActiveFocusItem.forceActiveFocus()
            PageController.forceStackActiveFocus()
        }
    }
}
