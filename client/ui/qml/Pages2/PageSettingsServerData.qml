import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"

PageType {
    id: root

    signal lastItemTabClickedSignal()

    onFocusChanged: content.isServerWithWriteAccess ?
                        labelWithButton.forceActiveFocus() :
                        labelWithButton3.forceActiveFocus()

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

        function onRemoveProcessedServerFinished(finishedMessage) {
            if (!ServersModel.getServersCount()) {
                PageController.replaceStartPage()
            } else {
                PageController.goToStartPage()
                PageController.goToPage(PageEnum.PageSettingsServersList)
            }
            PageController.showNotificationMessage(finishedMessage)
        }

        function onRebootProcessedServerFinished(finishedMessage) {
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

        function onProcessedServerIndexChanged() {
            content.isServerWithWriteAccess = ServersModel.isProcessedServerHasWriteAccess()
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

            property bool isServerWithWriteAccess: ServersModel.isProcessedServerHasWriteAccess()

            LabelWithButtonType {
                id: labelWithButton
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Clear Amnezia cache")
                descriptionText: qsTr("May be needed when changing other settings")

                KeyNavigation.tab: labelWithButton1

                clickedFunction: function() {
                    var headerText = qsTr("Clear cached profiles?")
                    var descriptionText = qsTr("")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        SettingsController.clearCachedProfiles()
                        PageController.showBusyIndicator(false)

                        if (!GC.isMobile()) {
                            labelWithButton.forceActiveFocus()
                        }                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            labelWithButton.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            LabelWithButtonType {
                id: labelWithButton1
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Check the server for previously installed Amnezia services")
                descriptionText: qsTr("Add them to the application if they were not displayed")

                KeyNavigation.tab: labelWithButton2

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
                id: labelWithButton2
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Reboot server")
                textColor: "#EB5757"

                KeyNavigation.tab: labelWithButton3

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
                        InstallController.rebootProcessedServer()
                        PageController.showBusyIndicator(false)

                        if (!GC.isMobile()) {
                            labelWithButton2.forceActiveFocus()
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            labelWithButton2.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            LabelWithButtonType {
                id: labelWithButton3
                Layout.fillWidth: true

                text: qsTr("Remove this server from the app")
                textColor: "#EB5757"

                Keys.onTabPressed: {
                    if (content.isServerWithWriteAccess) {
                        labelWithButton4.forceActiveFocus()
                    } else {
                        labelWithButton5.visible ?
                            labelWithButton5.forceActiveFocus() :
                            lastItemTabClickedSignal()
                    }
                }

                clickedFunction: function() {
                    var headerText = qsTr("Do you want to remove the server from application?")
                    var descriptionText = qsTr("All installed AmneziaVPN services will still remain on the server.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
                        }
                        InstallController.removeProcessedServer()
                        PageController.showBusyIndicator(false)

                        if (!GC.isMobile()) {
                            labelWithButton3.forceActiveFocus()
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            labelWithButton3.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {}

            LabelWithButtonType {
                id: labelWithButton4
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Clear server Amnezia-installed services")
                textColor: "#EB5757"

                Keys.onTabPressed: labelWithButton5.visible ?
                                    labelWithButton5.forceActiveFocus() :
                                    root.lastItemTabClickedSignal()

                clickedFunction: function() {
                    var headerText = qsTr("Do you want to clear server Amnezia-installed services?")
                    var descriptionText = qsTr("All containers will be deleted on the server. This means that configuration files, keys and certificates will be deleted.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
                        }
                        InstallController.removeAllContainers()
                        if (!GC.isMobile()) {
                            labelWithButton4.forceActiveFocus()
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            labelWithButton4.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            LabelWithButtonType {
                id: labelWithButton5
                visible: ServersModel.getProcessedServerData("isServerFromApi")
                Layout.fillWidth: true

                text: qsTr("Reset API config")
                textColor: "#EB5757"

                Keys.onTabPressed: root.lastItemTabClickedSignal()

                clickedFunction: function() {
                    var headerText = qsTr("Do you want to reset API config?")
                    var descriptionText = ""
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.showBusyIndicator(true)
                        ApiController.clearApiConfig()
                        PageController.showBusyIndicator(false)

                        if (!GC.isMobile()) {
                            labelWithButton5.forceActiveFocus()
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            labelWithButton5.forceActiveFocus()
                        }
                    }
                    
                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            DividerType {
                visible: ServersModel.getProcessedServerData("isServerFromApi")
            }
        }
    }
}
