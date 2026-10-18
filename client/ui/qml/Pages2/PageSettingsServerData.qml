import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"

PageType {
    id: root

    property bool isServerWithWriteAccess: ServersUiController.isProcessedServerHasWriteAccess()

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

        function onRebootServerFinished(finishedMessage) {
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
        target: ServersUiController

        function onProcessedServerIdChanged() {
            root.isServerWithWriteAccess = ServersUiController.isProcessedServerHasWriteAccess()
        }
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.isServerWithWriteAccess

                text: qsTr("Check the server for previously installed Amnezia services")
                subtitle: qsTr("Add them to the application if they were not displayed")

                onClicked: {
                    PageController.showBusyIndicator(true)
                    InstallController.scanServerForInstalledContainers(ServersUiController.processedServerId)
                    PageController.showBusyIndicator(false)
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.isServerWithWriteAccess

                text: qsTr("Reboot server")
                destructive: true

                onClicked: {
                    var headerText = qsTr("Do you want to reboot the server?")
                    var descriptionText = qsTr("The reboot process may take approximately 30 seconds. Are you sure you wish to proceed?")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot reboot server during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            InstallController.rebootServer(ServersUiController.processedServerId)
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {

                    }

                    Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Remove server from application")
                destructive: true

                onClicked: {
                    var headerText = qsTr("Do you want to remove the server from application?")
                    var descriptionText = qsTr("All installed AmneziaVPN services will still remain on the server.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot remove server during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            InstallController.removeServer(ServersUiController.processedServerId)
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {

                    }

                    Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.isServerWithWriteAccess

                text: qsTr("Clear server from Amnezia software")
                destructive: true

                onClicked: {
                    var headerText = qsTr("Do you want to clear server from Amnezia software?")
                    var descriptionText = qsTr("All users whom you shared a connection with will no longer be able to connect to it.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot clear server from Amnezia software during active connection"))
                        } else {
                            PageController.goToPage(PageEnum.PageDeinstalling)
                            InstallController.removeAllContainers(ServersUiController.processedServerId)
                        }
                    }
                    var noButtonFunction = function() {

                    }

                    Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ServersUiController.isServerFromApi(ServersUiController.processedServerId)

                text: qsTr("Reset API config")
                destructive: true

                onClicked: {
                    var headerText = qsTr("Do you want to reset API config?")
                    var descriptionText = ""
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot reset API config during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            SubscriptionUiController.removeApiConfig(ServersUiController.processedServerId)
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {

                    }

                    Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 16 + PageController.safeAreaBottomMargin
            }
        }
    }
}
