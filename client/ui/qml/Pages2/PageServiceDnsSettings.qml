import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ListViewType {
        id: listView

        anchors.fill: parent

        header: NavigationBarType {
            width: listView.width

            title: "AmneziaDNS"
            description: qsTr("A DNS service is installed on your server, and it is only accessible via VPN.\n").trim() + " " +
                         qsTr("The DNS address is the same as the address of your server. You can configure DNS in the settings, under the connections tab.")
        }

        model: 1

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            ListItemType {
                id: removeButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Remove ") + ContainersModel.getProcessedContainerName()
                destructive: true
                showSeparator: false

                onClicked: {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersUiController.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected
                                && SettingsController.isAmneziaDnsEnabled()) {
                            PageController.showNotificationMessage(qsTr("Cannot remove AmneziaDNS from running server"))
                        } else {
                            PageController.goToPage(PageEnum.PageDeinstalling)
                            InstallController.removeContainer(ServersUiController.processedServerId, ServersUiController.processedContainerIndex)
                        }
                    }
                    var noButtonFunction = function() {}

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }

            Item {
                Layout.preferredHeight: 16 + PageController.safeAreaBottomMargin
            }
        }
    }
}
