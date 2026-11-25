import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool isUnsupportedContainer: ContainerProps.isUnsupportedContainer(ServersUiController.processedContainerIndex)
    property bool isClearCacheVisible: !isUnsupportedContainer && ServersUiController.isProcessedServerHasWriteAccess() && !ContainersModel.isServiceContainer(ServersUiController.processedContainerIndex)
    property bool isOutdatedAwgContainer: ServersUiController.isProcessedContainerOutdatedAwg()

    ListViewType {
        id: listView

        anchors.fill: parent

        header: ColumnLayout {
            width: listView.width

            z: 2

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: ContainersModel.getProcessedContainerName() + qsTr(" settings")
                description: root.isUnsupportedContainer ? qsTr("This protocol is no longer supported.") : ""
            }

            NotificationType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                visible: root.isOutdatedAwgContainer

                variant: NotificationType.Warning
                text: qsTr("AmneziaWG 2.0 is outdated and does not include the latest security improvements, but it will continue to work. Moving to AmneziaWG 3.1 by deploying a new container on the server is recommended for stronger protocol security")
            }
        }

        model: root.isUnsupportedContainer ? null : ProtocolsModel

        delegate: ColumnLayout {
            id: delegateContent

            width: listView.width

            spacing: 0

            property bool isClientSettingsVisible: isWireGuard || isAwg
            property bool isServerSettingsVisible: ServersUiController.isProcessedServerHasWriteAccess() || isXray

            ListItemType {
                id: clientSettings

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: protocolName + qsTr(" connection settings")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                visible: delegateContent.isClientSettingsVisible

                onClicked: {
                    if (isClientProtocolExists) {
                        InstallController.openClientSettings(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, protocolIndex)
                        PageController.goToPage(clientProtocolPage);
                    } else {
                        PageController.showNotificationMessage(qsTr("Click the \"connect\" button to create a connection configuration"))
                    }
                }
            }

            ListItemType {
                id: serverSettings

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: protocolName + qsTr(" server settings")
                trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                visible: delegateContent.isServerSettingsVisible

                onClicked: {
                    InstallController.openServerSettings(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, protocolIndex)
                    PageController.goToPage(serverProtocolPage);
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            spacing: 0

            ListItemType {
                id: clearCacheButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: root.isClearCacheVisible

                text: qsTr("Clear profile")

                onClicked: {
                    var headerText = qsTr("Clear %1 profile?").arg(ContainersModel.getProcessedContainerName())
                    var descriptionText = qsTr("The connection configuration will be deleted for this device only")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                            var message = qsTr("Unable to clear %1 profile while there is an active connection").arg(ContainersModel.getProcessedContainerName())
                            PageController.showNotificationMessage(message)
                            return
                        }

                        PageController.showBusyIndicator(true)
                        InstallController.clearCachedProfile(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
                        PageController.showBusyIndicator(false)
                    }

                    var noButtonFunction = function() {
                    }

                    Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            ListItemType {
                id: removeButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ServersUiController.isProcessedServerHasWriteAccess()

                text: qsTr("Remove ")
                destructive: true

                onClicked: {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                    var descriptionText = qsTr("All users with whom you shared a connection will no longer be able to connect to it.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected
                                && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                            PageController.showNotificationMessage(qsTr("Cannot remove active container"))
                        } else
                        {
                            PageController.goToPage(PageEnum.PageDeinstalling)
                            InstallController.removeContainer(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
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
