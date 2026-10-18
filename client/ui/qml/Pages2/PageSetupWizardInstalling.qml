import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Component.onCompleted: {
        root.installingContainerIndex = ServersUiController.processedContainerIndex
        PageController.disableTabBar(true)
    }
    Component.onDestruction: PageController.disableTabBar(false)

    property bool isTimerRunning: true
    property string progressBarText: qsTr("Usually it takes no more than 5 minutes")
    property bool isCancelButtonVisible: false
    property int installingContainerIndex: -1

    Connections {
        target: InstallController

        function onInstallContainerFinished(finishedMessage, isServiceInstall) {
            PageController.closePage() // close installing page
            PageController.closePage() // close protocol settings page

            if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageHome)) {
                PageController.restorePageHomeState(true)
            }

            if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageSetupWizardProtocols)) {
                PageController.goToPage(PageEnum.PageHome)
            }

            PageController.showNotificationMessage(finishedMessage)
        }

        function onInstallServerFinished(finishedMessage) {
            PageController.goToPageHome()
            PageController.showNotificationMessage(finishedMessage)
        }

        function onServerAlreadyExists(serverIndex) {
            PageController.goToStartPage()
            ServersUiController.setProcessedServerId(ServersUiController.getServerId(serverIndex))
            PageController.goToPage(PageEnum.PageSettingsServerInfo, false)

            PageController.showErrorMessage(qsTr("The server has already been added to the application"))
        }

        function onServerIsBusy(isBusy) {
            if (isBusy) {
                root.isCancelButtonVisible = true
                root.progressBarText = qsTr("Amnezia has detected that your server is currently ") +
                                       qsTr("busy installing other software. Amnezia installation ") +
                                       qsTr("will pause until the server finishes installing other software")
                root.isTimerRunning = false
            } else {
                root.isCancelButtonVisible = false
                root.progressBarText = qsTr("Usually it takes no more than 5 minutes")
                root.isTimerRunning = true
            }
        }
    }

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "dockerContainer"
                value: root.installingContainerIndex
            }
        ]
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        model: proxyContainersModel

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                backButtonVisible: false

                title: qsTr("Installing")
                description: name
            }

            ProgressBarType {
                id: progressBar

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                Timer {
                    id: timer

                    interval: 300
                    repeat: true
                    running: root.isTimerRunning
                    onTriggered: {
                        progressBar.value += 0.003
                    }
                }
            }

            SmallTextType {
                id: progressText

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: root.progressBarText
            }

            ButtonType {
                id: cancelIntallationButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.isCancelButtonVisible

                text: qsTr("Cancel installation")

                onClicked: {
                    InstallController.cancelInstallation()
                    PageController.showBusyIndicator(true)
                }
            }
        }
    }
}
