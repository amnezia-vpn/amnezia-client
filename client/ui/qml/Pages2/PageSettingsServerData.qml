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
                message = qsTr("No new installed containers found")
            }

            PageController.showErrorMessage(message)
        }

        function onRemoveCurrentlyProcessedServerFinished(finishedMessage) {
            if (!ServersModel.getServersCount()) {
                PageController.replaceStartPage()
            } else {
                PageController.goToStartPage()
                PageController.goToPage(PageEnum.PageSettingsServersList)
            }
            PageController.showNotificationMessage(finishedMessage)
        }

        function onRebootCurrentlyProcessedServerFinished(finishedMessage) {
            PageController.showNotificationMessage(finishedMessage)
        }

        function onRemoveAllContainersFinished(finishedMessage) {
            PageController.closePage() // close deInstalling page
            PageController.showNotificationMessage(finishedMessage)
        }

        function onRemoveCurrentlyProcessedContainerFinished(finishedMessage) {
            PageController.closePage() // close deInstalling page
            PageController.closePage() // close page with remove button
            PageController.showNotificationMessage(finishedMessage)
        }
    }

    Connections {
        target: SettingsController
        function onChangeSettingsFinished(finishedMessage) {
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
                    questionDrawer.descriptionText = qsTr("")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        PageController.showBusyIndicator(true)
                        SettingsController.clearCachedProfiles()
                        PageController.showBusyIndicator(false)
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

                text: qsTr("Check the server for previously installed Amnezia services")
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
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Reboot server")
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Do you want to reboot the server?")
                    questionDrawer.descriptionText = qsTr("The reboot process may take approximately 30 seconds. Are you sure you wish to proceed?")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        PageController.showBusyIndicator(true)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
                        }
                        InstallController.rebootCurrentlyProcessedServer()
                        PageController.showBusyIndicator(false)
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
                Layout.fillWidth: true

                text: qsTr("Remove server from application")
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Do you want to remove the server from application?")
                    questionDrawer.descriptionText = qsTr("All installed AmneziaVPN services will still remain on the server.")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        PageController.showBusyIndicator(true)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
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
                    questionDrawer.headerText = qsTr("Do you want to clear server from Amnezia software?")
                    questionDrawer.descriptionText = qsTr("All containers will be deleted on the server. This means that configuration files, keys and certificates will be deleted.")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
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

            LabelWithButtonType {
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Clear server from Amnezia software")
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Do you want to clear server from Amnezia software?")
                    questionDrawer.descriptionText = qsTr("All containers will be deleted on the server. This means that configuration files, keys and certificates will be deleted.")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
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

            LabelWithButtonType {
                visible: ServersModel.isCurrentlyProcessedServerFromApi()
                Layout.fillWidth: true

                text: qsTr("Reset API config")
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Do you want to reset API config?")
                    questionDrawer.descriptionText = ""
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        PageController.showBusyIndicator(true)
                        ApiController.clearApiConfig()
                        PageController.showBusyIndicator(false)
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }

            DividerType {
                visible: ServersModel.isCurrentlyProcessedServerFromApi()
            }

            QuestionDrawer {
                id: questionDrawer
            }
        }
    }
}
