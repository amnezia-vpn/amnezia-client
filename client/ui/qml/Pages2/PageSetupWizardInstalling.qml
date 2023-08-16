import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Component.onCompleted: PageController.enableTabBar(false)
    Component.onDestruction: PageController.enableTabBar(true)

    property bool isTimerRunning: true
    property string progressBarText: qsTr("Usually it takes no more than 5 minutes")

    Connections {
        target: InstallController

        function onInstallContainerFinished(finishedMessage, isServiceInstall) {
            goToStartPage()
            if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageHome)) {
                PageController.restorePageHomeState(true)
            } else if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageSettings)) {
                goToPage(PageEnum.PageSettingsServersList, false)
                goToPage(PageEnum.PageSettingsServerInfo, false)
                if (isServiceInstall) {
                    PageController.goToPageSettingsServerServices()
                }
            } else {
                goToPage(PageEnum.PageHome)
            }

            PageController.showNotificationMessage(finishedMessage)
        }

        function onInstallServerFinished(finishedMessage) {
            goToStartPage()
            if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageHome)) {
                PageController.restorePageHomeState()
            } else if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageSettings)) {
                goToPage(PageEnum.PageSettingsServersList, false)
            } else {
                PageController.replaceStartPage()
            }

            PageController.showNotificationMessage(finishedMessage)
        }

        function onServerAlreadyExists(serverIndex) {
            goToStartPage()
            ServersModel.currentlyProcessedIndex = serverIndex
            goToPage(PageEnum.PageSettingsServerInfo, false)

            PageController.showErrorMessage(qsTr("The server has already been added to the application"))
        }

        function onServerIsBusy(isBusy) {
            if (isBusy) {
                root.progressBarText = qsTr("Amnesia has detected that your server is currently ") +
                                       qsTr("busy installing other software. Amnesia installation ") +
                                       qsTr("will pause until the server finishes installing other software")
                root.isTimerRunning = false
            } else {
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
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]
    }

    FlickableType {
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 16

            ListView {
                id: container
                width: parent.width
                height: container.contentItem.height
                currentIndex: -1
                clip: true
                interactive: false
                model: proxyContainersModel

                delegate: Item {
                    implicitWidth: container.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16

                        HeaderType {
                            Layout.fillWidth: true
                            Layout.topMargin: 20

                            headerText: qsTr("Installing")
                            descriptionText: name
                        }

                        ProgressBarType {
                            id: progressBar

                            Layout.fillWidth: true
                            Layout.topMargin: 32

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

                        ParagraphTextType {
                            id: progressText

                            Layout.fillWidth: true
                            Layout.topMargin: 8

                            text: root.progressBarText
                        }
                    }
                }
            }
        }
    }
}
