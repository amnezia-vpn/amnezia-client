import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    Connections {
        target: InstallController

        function onScanServerFinished(isInstalledContainerFound) {
            var message = ""
            if (isInstalledContainerFound) {
                message = qsTr("All installed containers have been added to the application")
            } else {
                message = qsTr("No installed containers found")
            }

            PageController.showErrorMessage(message)
        }

        function onInstallationErrorOccurred(errorMessage) {
            closePage() // close deInstalling page
            PageController.showErrorMessage(errorMessage)
        }

        function onRemoveCurrentlyProcessedServerFinished(finishedMessage) {
            if (!ServersModel.getServersCount()) {
                PageController.replaceStartPage()
            } else {
                goToStartPage()
                goToPage(PageEnum.PageSettingsServersList)
            }
            PageController.showNotificationMessage(finishedMessage)
        }

        function onRemoveAllContainersFinished(finishedMessage) {
            closePage() // close deInstalling page
            PageController.showNotificationMessage(finishedMessage)
        }

        function onRemoveCurrentlyProcessedContainerFinished(finishedMessage) {
            closePage() // close deInstalling page
            closePage() // close page with remove button
            PageController.showNotificationMessage(finishedMessage)
        }
    }

    Connections {
        target: ServersModel

        function onCurrentlyProcessedServerIndexChanged() {
            content.isServerWithWriteAccess = ServersModel.isCurrentlyProcessedServerHasWriteAccess()
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

            property bool isServerWithWriteAccess: ServersModel.isCurrentlyProcessedServerHasWriteAccess()

            LabelWithButtonType {
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Clear Amnezia cache")
                descriptionText: qsTr("May be needed when changing other settings")

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Clear cached profiles?")
                    questionDrawer.descriptionText = qsTr("some description")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        ContainersModel.clearCachedProfiles()
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            LabelWithButtonType {
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Check the server for previously installed Amnesia services")
                descriptionText: qsTr("Add them to the application if they were not displayed")

                clickedFunction: function() {
                    PageController.showBusyIndicator(true)
                    InstallController.scanServerForInstalledContainers()
                    PageController.showBusyIndicator(false)
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Remove server from application")
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Remove server?")
                    questionDrawer.descriptionText = qsTr("All installed AmneziaVPN services will still remain on the server.")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        PageController.showBusyIndicator(true)
                        if (ServersModel.isDefaultServerCurrentlyProcessed && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
                        }
                        InstallController.removeCurrentlyProcessedServer()
                        PageController.showBusyIndicator(false)
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }

            DividerType {}

            LabelWithButtonType {
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Clear server from Amnezia software")
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Clear server from Amnezia software?")
                    questionDrawer.descriptionText = qsTr(" All containers will be deleted on the server. This means that configuration files, keys and certificates will be deleted.")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        goToPage(PageEnum.PageDeinstalling)
                        if (ServersModel.isDefaultServerCurrentlyProcessed && ConnectionController.isConnected) {
                            ConnectionController.closeVpnConnection()
                        }
                        InstallController.removeAllContainers()
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            QuestionDrawer {
                id: questionDrawer
            }
        }
    }
}
