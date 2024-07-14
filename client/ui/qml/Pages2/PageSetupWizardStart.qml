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

    defaultActiveFocusItem: focusItem

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
            PageController.goToPageHome()
        }
    }

    Connections {
        target: ImportController

        function onRestoreAppConfig(data) {
            PageController.showBusyIndicator(true)
            SettingsController.restoreAppConfigFromData(data)
            PageController.showBusyIndicator(false)
        }
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 0

        Image {
            id: image
            source: "qrc:/images/amneziaBigLogo.png"

            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.topMargin: 32
            Layout.preferredWidth: 360
            Layout.preferredHeight: 287
        }

        ParagraphTextType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            text: qsTr("Free service for creating a personal VPN on your server.") +
                  qsTr(" Helps you access blocked content without revealing your privacy, even to VPN providers.")
        }

        Item {
            id: focusItem
            KeyNavigation.tab: startButton
            Layout.fillHeight: true
        }

        BasicButtonType {
            id: startButton
            Layout.fillWidth: true
            Layout.bottomMargin: 48
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.alignment: Qt.AlignBottom

            text: qsTr("Let's get started")

            clickedFunc: function() {
                PageController.goToPage(PageEnum.PageSetupWizardConfigSource)
            }

            Keys.onTabPressed: lastItemTabClicked(focusItem)
        }
    }
}
