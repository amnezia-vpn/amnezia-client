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
                    var headerText = qsTr("Clear cached profiles?")
                    var descriptionText = qsTr("")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        SettingsController.clearCachedProfiles()
                        PageController.showBusyIndicator(false)
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
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
                    var headerText = qsTr("Do you want to reboot the server?")
                    var descriptionText = qsTr("The reboot process may take approximately 30 seconds. Are you sure you wish to proceed?")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
                        }
                        InstallController.rebootCurrentlyProcessedServer()
                        PageController.showBusyIndicator(false)
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
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
                    var headerText = qsTr("Do you want to remove the server?")
                    var descriptionText = qsTr("All installed AmneziaVPN services will still remain on the server.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
                        }
                        InstallController.removeCurrentlyProcessedServer()
                        PageController.showBusyIndicator(false)
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {}

            LabelWithButtonType {
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Clear server from Amnezia software")
                textColor: "#EB5757"

                clickedFunction: function() {
                    var headerText = qsTr("Do you want to clear server from Amnezia software?")
                    var descriptionText = qsTr("All containers will be deleted on the server. This means that configuration files, keys and certificates will be deleted.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
                        }
                        InstallController.removeAllContainers()
                    }
                    var noButtonFunction = function() {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }
        }
    }
}
