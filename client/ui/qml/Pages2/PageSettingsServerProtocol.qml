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

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
        
        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 32

                headerText: ContainersModel.getProcessedContainerName() + qsTr(" settings")
                descriptionText: root.isUnsupportedContainer ? qsTr("This protocol is no longer supported.") : ""
            }
        }

        model: root.isUnsupportedContainer ? null : ProtocolsModel

        delegate: ColumnLayout {
            id: delegateContent

            width: listView.width

            property bool isClientSettingsVisible: isWireGuard || isAwg
            property bool isServerSettingsVisible: ServersUiController.isProcessedServerHasWriteAccess()

            LabelWithButtonType {
                id: clientSettings

                Layout.fillWidth: true

                text: protocolName + qsTr(" connection settings")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                visible: delegateContent.isClientSettingsVisible

                clickedFunction: function() {
                    if (isClientProtocolExists) {
                        InstallController.openClientSettings(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, protocolIndex)
                        PageController.goToPage(clientProtocolPage);
                    } else {
                        PageController.showNotificationMessage(qsTr("Click the \"connect\" button to create a connection configuration"))
                    }
                }

                MouseArea {
                    anchors.fill: clientSettings
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {
                visible: delegateContent.isClientSettingsVisible
            }

            LabelWithButtonType {
                id: serverSettings

                Layout.fillWidth: true

                text: protocolName + qsTr(" server settings")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                visible: delegateContent.isServerSettingsVisible

                clickedFunction: function() {
                    InstallController.openServerSettings(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, protocolIndex)
                    PageController.goToPage(serverProtocolPage);
                }

                MouseArea {
                    anchors.fill: serverSettings
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {
                visible: delegateContent.isServerSettingsVisible
            }
        }

        footer: ColumnLayout {

            width: listView.width

            LabelWithButtonType {
                id: clearCacheButton

                Layout.fillWidth: true

                visible: root.isClearCacheVisible

                text: qsTr("Clear profile")

                clickedFunction: function() {
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

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: clearCacheButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {
                visible: root.isClearCacheVisible
            }

            LabelWithButtonType {
                id: removeButton

                Layout.fillWidth: true

                visible: ServersUiController.isProcessedServerHasWriteAccess()

                text: qsTr("Remove ")
                textColor: AmneziaStyle.color.vibrantRed

                clickedFunction: function() {
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

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: removeButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {
                visible: ServersUiController.isProcessedServerHasWriteAccess()
            }
        }
    }
}
