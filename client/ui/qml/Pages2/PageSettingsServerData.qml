import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"

PageType {
    id: root

    signal lastItemTabClickedSignal()

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

        function onRebootProcessedServerFinished(finishedMessage) {
            PageController.showNotificationMessage(finishedMessage)
        }

        function onRemoveAllContainersFinished(finishedMessage) {
            PageController.closePage() // close deInstalling page
            PageController.showNotificationMessage(finishedMessage)
        }

        function onRemoveProcessedContainerFinished(finishedMessage) {
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
            listView.isServerWithWriteAccess = ServersModel.isProcessedServerHasWriteAccess()
        }
    }

    ListViewType {
        id: listView

        property bool isServerWithWriteAccess: ServersModel.isProcessedServerHasWriteAccess()

        anchors.fill: parent

        model: serverActions

        delegate: ColumnLayout {
            width: listView.width

            LabelWithButtonType {
                Layout.fillWidth: true

                visible: isVisible

                text: title
                descriptionText: description
                textColor: tColor

                clickedFunction: function() {
                    clickedHandler()
                }
            }

            DividerType {
                visible: isVisible
            }
        }
    }

    property list<QtObject> serverActions: [
        check,
        reboot,
        remove,
        clear,
        reset,
        switch_to_premium,
    ]

    QtObject {
        id: check

        property bool isVisible: true
        readonly property string title: qsTr("Check the server for previously installed Amnezia services")
        readonly property string description: qsTr("Add them to the application if they were not displayed")
        readonly property var tColor: AmneziaStyle.color.paleGray
        readonly property var clickedHandler: function() {
            PageController.showBusyIndicator(true)
            InstallController.scanServerForInstalledContainers()
            PageController.showBusyIndicator(false)
        }
    }

    QtObject {
        id: reboot

        property bool isVisible: true
        readonly property string title: qsTr("Reboot server")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = qsTr("Do you want to reboot the server?")
            var descriptionText = qsTr("The reboot process may take approximately 30 seconds. Are you sure you wish to proceed?")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot reboot server during active connection"))
                } else {
                    PageController.showBusyIndicator(true)
                    InstallController.rebootProcessedServer()
                    PageController.showBusyIndicator(false)
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }

    QtObject {
        id: remove

        property bool isVisible: true
        readonly property string title: qsTr("Remove server from application")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = qsTr("Do you want to remove the server from application?")
            var descriptionText = qsTr("All installed AmneziaVPN services will still remain on the server.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot remove server during active connection"))
                } else {
                    PageController.showBusyIndicator(true)
                    InstallController.removeProcessedServer()
                    PageController.showBusyIndicator(false)
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }

    QtObject {
        id: clear

        property bool isVisible: true
        readonly property string title: qsTr("Clear server from Amnezia software")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = qsTr("Do you want to clear server from Amnezia software?")
            var descriptionText = qsTr("All users whom you shared a connection with will no longer be able to connect to it.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot clear server from Amnezia software during active connection"))
                } else {
                    PageController.goToPage(PageEnum.PageDeinstalling)
                    InstallController.removeAllContainers()
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }

    QtObject {
        id: reset

        property bool isVisible: ServersModel.getProcessedServerData("isServerFromTelegramApi")
        readonly property string title: qsTr("Reset API config")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            var headerText = qsTr("Do you want to reset API config?")
            var descriptionText = ""
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                    PageController.showNotificationMessage(qsTr("Cannot reset API config during active connection"))
                } else {
                    PageController.showBusyIndicator(true)
                    InstallController.removeApiConfig(ServersModel.processedIndex)
                    PageController.showBusyIndicator(false)
                }
            }
            var noButtonFunction = function() {

            }

            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }

    QtObject {
        id: switch_to_premium

        property bool isVisible: ServersModel.getProcessedServerData("isServerFromTelegramApi")
        readonly property string title: qsTr("Switch to the new Amnezia Premium subscription")
        readonly property string description: ""
        readonly property var tColor: AmneziaStyle.color.vibrantRed
        readonly property var clickedHandler: function() {
            PageController.goToPageHome()
            ApiPremV1MigrationController.showMigrationDrawer()
        }
    }
}
